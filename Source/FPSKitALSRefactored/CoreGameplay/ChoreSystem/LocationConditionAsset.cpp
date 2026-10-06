#include "LocationConditionAsset.h"
#include "../LocationTrackerSubsystem/LocationTrackerSubsystem.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/WorldMapAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "UObject/SoftObjectPath.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательное
// ─────────────────────────────────────────────────────────────────────────────
static bool MatchesDisplayName(const UObject* Asset, const FText& InDisplayName)
{
    if (!Asset) return false;

    const FString Target = InDisplayName.ToString();
    if (Target.IsEmpty()) return false;

    // Сначала пробуем FText DisplayName. У всех наших ассетов (Map/Region/
    // Street/InteriorSet/Floor) DisplayName это FText.
    if (FProperty* Prop = Asset->GetClass()->FindPropertyByName(TEXT("DisplayName")))
    {
        if (const FText* DisplayNameProp = Prop->ContainerPtrToValuePtr<FText>(Asset))
        {
            if (!DisplayNameProp->IsEmpty())
                return DisplayNameProp->ToString().Equals(Target, ESearchCase::CaseSensitive);
        }
    }

    // Fallback — по имени ассета.
    return Asset->GetName().Equals(Target, ESearchCase::CaseSensitive);
}

template<typename TAsset>
static void CollectAllAssetsOfClass(TArray<TAsset*>& OutAssets, const TCHAR* ClassName)
{
    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;
    ARM.Get().GetAssetsByClass(
        FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), ClassName),
        AssetDataList, true);

    for (const FAssetData& AD : AssetDataList)
    {
        if (TAsset* Asset = Cast<TAsset>(AD.ToSoftObjectPath().TryLoad()))
            OutAssets.Add(Asset);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
ULocationTrackerSubsystem* ULocationConditionAsset::FindTracker()
{
    if (!GEngine) return nullptr;

    const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
    for (const FWorldContext& C : Contexts)
    {
        UWorld* W = C.World();
        if (!W || !W->IsGameWorld()) continue;

        UGameInstance* GI = W->GetGameInstance();
        if (!GI) continue;

        if (ULocationTrackerSubsystem* T = GI->GetSubsystem<ULocationTrackerSubsystem>())
            return T;
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
void ULocationConditionAsset::ResolveTargets()
{
    CachedTargetPackageNames.Reset();

    const FString Target = TargetDisplayName.ToString();
    if (Target.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationConditionAsset '%s': TargetDisplayName is empty"),
            *GetName());
        return;
    }

    auto AddUniquePackage = [this](const FString& Pkg)
        {
            const FString Norm = ULocationTrackerSubsystem::NormalizeLevelName(Pkg);
            if (!Norm.IsEmpty())
                CachedTargetPackageNames.AddUnique(Norm);
        };

    int32 MatchCount = 0;

    // ── Floor ────────────────────────────────────────────────────────────
    {
        TArray<UFloorAsset*> All;
        CollectAllAssetsOfClass(All, TEXT("FloorAsset"));
        for (UFloorAsset* F : All)
        {
            if (!MatchesDisplayName(F, TargetDisplayName)) continue;
            ++MatchCount;
            if (!F->FloorLevel.IsNull())
                AddUniquePackage(F->FloorLevel.ToSoftObjectPath().GetLongPackageName());
        }
    }

    // ── Building (InteriorSetAsset) — все этажи ──────────────────────────
    {
        TArray<UInteriorSetAsset*> All;
        CollectAllAssetsOfClass(All, TEXT("InteriorSetAsset"));
        for (UInteriorSetAsset* B : All)
        {
            if (!MatchesDisplayName(B, TargetDisplayName)) continue;
            ++MatchCount;
            for (const TSoftObjectPtr<UFloorAsset>& FloorRef : B->Floors)
            {
                if (UFloorAsset* F = FloorRef.LoadSynchronous())
                {
                    if (!F->FloorLevel.IsNull())
                        AddUniquePackage(F->FloorLevel.ToSoftObjectPath().GetLongPackageName());
                }
            }
        }
    }

    // ── Street — все этажи всех зданий улицы ─────────────────────────────
    {
        TArray<UStreetAsset*> All;
        CollectAllAssetsOfClass(All, TEXT("StreetAsset"));
        for (UStreetAsset* S : All)
        {
            if (!MatchesDisplayName(S, TargetDisplayName)) continue;
            ++MatchCount;
            for (const TSoftObjectPtr<UInteriorSetAsset>& BuildingRef : S->InteriorSets)
            {
                if (UInteriorSetAsset* B = BuildingRef.LoadSynchronous())
                {
                    for (const TSoftObjectPtr<UFloorAsset>& FloorRef : B->Floors)
                    {
                        if (UFloorAsset* F = FloorRef.LoadSynchronous())
                        {
                            if (!F->FloorLevel.IsNull())
                                AddUniquePackage(F->FloorLevel.ToSoftObjectPath().GetLongPackageName());
                        }
                    }
                }
            }
        }
    }

    // ── Region — сцена региона ───────────────────────────────────────────
    {
        TArray<UWorldRegionAsset*> All;
        CollectAllAssetsOfClass(All, TEXT("WorldRegionAsset"));
        for (UWorldRegionAsset* R : All)
        {
            if (!MatchesDisplayName(R, TargetDisplayName)) continue;
            ++MatchCount;
            if (!R->RegionLevel.IsNull())
                AddUniquePackage(R->RegionLevel.ToSoftObjectPath().GetLongPackageName());
        }
    }

    // ── Map — все регионы карты ──────────────────────────────────────────
    {
        TArray<UWorldMapAsset*> All;
        CollectAllAssetsOfClass(All, TEXT("WorldMapAsset"));
        for (UWorldMapAsset* M : All)
        {
            if (!MatchesDisplayName(M, TargetDisplayName)) continue;
            ++MatchCount;
            for (const TSoftObjectPtr<UWorldRegionAsset>& RegionRef : M->Regions)
            {
                if (UWorldRegionAsset* R = RegionRef.LoadSynchronous())
                {
                    if (!R->RegionLevel.IsNull())
                        AddUniquePackage(R->RegionLevel.ToSoftObjectPath().GetLongPackageName());
                }
            }
        }
    }

    if (MatchCount == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationConditionAsset '%s': no asset found with DisplayName '%s'"),
            *GetName(), *Target);
    }
    else if (MatchCount > 1)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationConditionAsset '%s': DisplayName '%s' matched %d assets, using all"),
            *GetName(), *Target, MatchCount);
    }

    UE_LOG(LogTemp, Log,
        TEXT("LocationConditionAsset '%s': DisplayName '%s' resolved to %d package(s): [%s]"),
        *GetName(), *Target,
        CachedTargetPackageNames.Num(),
        *FString::Join(CachedTargetPackageNames, TEXT(", ")));
}

// ─────────────────────────────────────────────────────────────────────────────
void ULocationConditionAsset::CompileCondition()
{
    // Резолвим имена пакетов один раз при компиляции.
    ResolveTargets();

    class FLocationCondition : public IOutcomeCondition
    {
    public:
        FLocationCondition(const ULocationConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("Location: Invalid");

            const FString QueryStr = StaticEnum<ELocationQueryType>()
                ->GetValueAsString(Asset->QueryType);

            const FString TargetStr = Asset->TargetDisplayName.ToString();

            if (Asset->CachedTargetPackageNames.Num() == 0)
            {
                return FString::Printf(TEXT("Location: [%s] '%s' [UNRESOLVED]"),
                    *QueryStr, *TargetStr);
            }

            const FString PackagesStr = FString::Join(Asset->CachedTargetPackageNames, TEXT(", "));

            switch (Asset->QueryType)
            {
            case ELocationQueryType::HasEnteredEver:
            case ELocationQueryType::LeftAndReturned:
                return FString::Printf(TEXT("Location: [%s] '%s' -> {%s}"),
                    *QueryStr, *TargetStr, *PackagesStr);

            case ELocationQueryType::VisitCountAtLeast:
                return FString::Printf(TEXT("Location: [%s] '%s' -> {%s}, count %s %d"),
                    *QueryStr, *TargetStr, *PackagesStr,
                    *StaticEnum<ECheckCompareOp>()->GetValueAsString(Asset->CompareOp),
                    Asset->VisitThreshold);

            default:
                return FString::Printf(TEXT("Location: [%s] '%s'"), *QueryStr, *TargetStr);
            }
        }

    private:
        const ULocationConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FLocationCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

// ─────────────────────────────────────────────────────────────────────────────
bool ULocationConditionAsset::EvaluateCondition(const FOutcomeEventBase& /*Outcome*/) const
{
    if (CachedTargetPackageNames.Num() == 0)
    {
        // CompileCondition либо не вызывался, либо ничего не зарезолвил.
        // Тихий false — все предупреждения уже выданы в CompileCondition.
        return false;
    }

    ULocationTrackerSubsystem* Tracker = FindTracker();
    if (!Tracker) return false;

    switch (QueryType)
    {
    case ELocationQueryType::HasEnteredEver:
    {
        for (const FString& Pkg : CachedTargetPackageNames)
        {
            if (Tracker->HasEnteredEver(Pkg))
                return true;
        }
        return false;
    }

    case ELocationQueryType::LeftAndReturned:
    {
        for (const FString& Pkg : CachedTargetPackageNames)
        {
            if (Tracker->HasLeftAndReturned(Pkg, 0.0f))
                return true;
        }
        return false;
    }

    case ELocationQueryType::VisitCountAtLeast:
    {
        int32 Total = 0;
        for (const FString& Pkg : CachedTargetPackageNames)
        {
            Total += Tracker->GetEnterCount(Pkg);
        }

        switch (CompareOp)
        {
        case ECheckCompareOp::Equal:          return Total == VisitThreshold;
        case ECheckCompareOp::NotEqual:       return Total != VisitThreshold;
        case ECheckCompareOp::Less:           return Total < VisitThreshold;
        case ECheckCompareOp::LessOrEqual:    return Total <= VisitThreshold;
        case ECheckCompareOp::Greater:        return Total > VisitThreshold;
        case ECheckCompareOp::GreaterOrEqual: return Total >= VisitThreshold;
        default:                              return false;
        }
    }

    default:
        return false;
    }
}