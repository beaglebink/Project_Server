#include "LocationConditionAsset.h"
#include "../LocationTrackerSubsystem/LocationTrackerSubsystem.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/WorldMapAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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
bool ULocationConditionAsset::ValidateHierarchy(FString& OutError) const
{
    // Проверяем по цепочке, если указано более одного уровня.
    if (TargetFloor && TargetBuilding)
    {
        UInteriorSetAsset* Parent = TargetFloor->ParentInteriorSet.LoadSynchronous();
        if (Parent != TargetBuilding)
        {
            OutError = FString::Printf(
                TEXT("TargetFloor '%s' does not belong to TargetBuilding '%s'"),
                *TargetFloor->GetName(), *TargetBuilding->GetName());
            return false;
        }
    }

    if (TargetBuilding && TargetStreet)
    {
        UStreetAsset* Parent = TargetBuilding->ParentStreet.LoadSynchronous();
        if (Parent != TargetStreet)
        {
            OutError = FString::Printf(
                TEXT("TargetBuilding '%s' does not belong to TargetStreet '%s'"),
                *TargetBuilding->GetName(), *TargetStreet->GetName());
            return false;
        }
    }

    if (TargetStreet && TargetRegion)
    {
        UWorldRegionAsset* Parent = TargetStreet->ParentWorldRegion.LoadSynchronous();
        if (Parent != TargetRegion)
        {
            OutError = FString::Printf(
                TEXT("TargetStreet '%s' does not belong to TargetRegion '%s'"),
                *TargetStreet->GetName(), *TargetRegion->GetName());
            return false;
        }
    }

    if (TargetRegion && TargetMap)
    {
        UWorldMapAsset* Parent = TargetRegion->ParentWorldMap.LoadSynchronous();
        if (Parent != TargetMap)
        {
            OutError = FString::Printf(
                TEXT("TargetRegion '%s' does not belong to TargetMap '%s'"),
                *TargetRegion->GetName(), *TargetMap->GetName());
            return false;
        }
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
bool ULocationConditionAsset::ResolveTargetPackageNames(TArray<FString>& OutPackageNames) const
{
    OutPackageNames.Reset();

    // ── Приоритет — самое глубокое указанное поле ────────────────────────
    // Floor > Building > Street > Region > Map.
    // Более общие поля (если заданы вместе с более глубокими) используются
    // только для проверки консистентности цепочки — это делает ValidateHierarchy,
    // здесь мы её не дублируем.

    // ── 1. Floor ─────────────────────────────────────────────────────────
    if (TargetFloor)
    {
        if (!TargetFloor->FloorLevel.IsNull())
        {
            const FString Norm = ULocationTrackerSubsystem::NormalizeLevelName(
                TargetFloor->FloorLevel.ToSoftObjectPath().GetLongPackageName());

            if (!Norm.IsEmpty())
            {
                OutPackageNames.AddUnique(Norm);
            }
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 2. Building — OR по всем этажам здания ───────────────────────────
    if (TargetBuilding)
    {
        for (const TSoftObjectPtr<UFloorAsset>& FloorRef : TargetBuilding->Floors)
        {
            UFloorAsset* Floor = FloorRef.LoadSynchronous();
            if (!Floor) continue;
            if (Floor->FloorLevel.IsNull()) continue;

            const FString Norm = ULocationTrackerSubsystem::NormalizeLevelName(
                Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName());

            if (!Norm.IsEmpty())
            {
                OutPackageNames.AddUnique(Norm);
            }
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 3. Street — OR по всем этажам всех зданий улицы ──────────────────
    if (TargetStreet)
    {
        for (const TSoftObjectPtr<UInteriorSetAsset>& BuildingRef : TargetStreet->InteriorSets)
        {
            UInteriorSetAsset* Building = BuildingRef.LoadSynchronous();
            if (!Building) continue;

            for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Building->Floors)
            {
                UFloorAsset* Floor = FloorRef.LoadSynchronous();
                if (!Floor) continue;
                if (Floor->FloorLevel.IsNull()) continue;

                const FString Norm = ULocationTrackerSubsystem::NormalizeLevelName(
                    Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName());

                if (!Norm.IsEmpty())
                {
                    OutPackageNames.AddUnique(Norm);
                }
            }
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 4. Region — одна сцена региона ───────────────────────────────────
    if (TargetRegion)
    {
        if (!TargetRegion->RegionLevel.IsNull())
        {
            const FString Norm = ULocationTrackerSubsystem::NormalizeLevelName(
                TargetRegion->RegionLevel.ToSoftObjectPath().GetLongPackageName());

            if (!Norm.IsEmpty())
            {
                OutPackageNames.AddUnique(Norm);
            }
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 5. Map — OR по сценам всех регионов карты ────────────────────────
    if (TargetMap)
    {
        for (const TSoftObjectPtr<UWorldRegionAsset>& RegionRef : TargetMap->Regions)
        {
            UWorldRegionAsset* Region = RegionRef.LoadSynchronous();
            if (!Region) continue;
            if (Region->RegionLevel.IsNull()) continue;

            const FString Norm = ULocationTrackerSubsystem::NormalizeLevelName(
                Region->RegionLevel.ToSoftObjectPath().GetLongPackageName());

            if (!Norm.IsEmpty())
            {
                OutPackageNames.AddUnique(Norm);
            }
        }
        return OutPackageNames.Num() > 0;
    }

    UE_LOG(LogTemp, Warning, TEXT("LocationConditionAsset '%s': ResolveTargetPackageNames -> [%s]"),
        *GetName(), *FString::Join(OutPackageNames, TEXT(", ")));
    return OutPackageNames.Num() > 0;

    // ── Ничего не указано ────────────────────────────────────────────────
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
void ULocationConditionAsset::CompileCondition()
{
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

            // Что указано в качестве цели — по самому глубокому полю.
            FString TargetDesc = TEXT("<no target>");
            if (Asset->TargetFloor)
                TargetDesc = FString::Printf(TEXT("Floor '%s'"), *Asset->TargetFloor->GetName());
            else if (Asset->TargetBuilding)
                TargetDesc = FString::Printf(TEXT("Building '%s' (any floor)"), *Asset->TargetBuilding->GetName());
            else if (Asset->TargetStreet)
                TargetDesc = FString::Printf(TEXT("Street '%s' (any building/floor)"), *Asset->TargetStreet->GetName());
            else if (Asset->TargetRegion)
                TargetDesc = FString::Printf(TEXT("Region '%s'"), *Asset->TargetRegion->GetName());
            else if (Asset->TargetMap)
                TargetDesc = FString::Printf(TEXT("Map '%s' (any region)"), *Asset->TargetMap->GetName());

            // Проверка консистентности цепочки — выводим предупреждение прямо в описание,
            // чтобы дизайнер увидел его в редакторе.
            FString HierarchyError;
            const bool bHierarchyOk = Asset->ValidateHierarchy(HierarchyError);
            const FString HierarchyNote = bHierarchyOk
                ? FString()
                : FString::Printf(TEXT(" [INCONSISTENT: %s]"), *HierarchyError);

            switch (Asset->QueryType)
            {
            case ELocationQueryType::CurrentlyAtTarget:
            case ELocationQueryType::HasEnteredEver:
                return FString::Printf(TEXT("Location: [%s] %s%s"),
                    *QueryStr, *TargetDesc, *HierarchyNote);

            case ELocationQueryType::HasEnteredRecently:
            case ELocationQueryType::LeftAndReturned:
                return FString::Printf(TEXT("Location: [%s] %s, window %g min%s"),
                    *QueryStr, *TargetDesc, Asset->TimeWindowMinutes, *HierarchyNote);

            case ELocationQueryType::VisitCountAtLeast:
                return FString::Printf(TEXT("Location: [%s] %s, threshold %d%s"),
                    *QueryStr, *TargetDesc, Asset->VisitThreshold, *HierarchyNote);

            default:
                return FString::Printf(TEXT("Location: [%s]%s"), *QueryStr, *HierarchyNote);
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
    // ── 1. Валидация цепочки иерархии ────────────────────────────────────
    // Если дизайнер указал несогласованные ассеты (Floor не из указанного
    // Building, Building не из указанной Street и т.д.) — условие всегда false.
    FString HierarchyError;
    if (!ValidateHierarchy(HierarchyError))
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationConditionAsset '%s': hierarchy INCONSISTENT — %s"),
            *GetName(), *HierarchyError);
        return false;
    }

    // ── 2. Достаём трекер ────────────────────────────────────────────────
    ULocationTrackerSubsystem* Tracker = FindTracker();
    if (!Tracker)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationConditionAsset '%s': Tracker NOT FOUND"),
            *GetName());
        return false;
    }

    // ── 3. Резолвим список целевых пакетов ───────────────────────────────
    TArray<FString> TargetPackageNames;
    if (!ResolveTargetPackageNames(TargetPackageNames) || TargetPackageNames.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationConditionAsset '%s': ResolveTargetPackageNames returned EMPTY"),
            *GetName());
        return false;
    }

    // ── 4. Диагностический лог перед проверкой ───────────────────────────
    UE_LOG(LogTemp, Warning,
        TEXT("LocationConditionAsset '%s': QueryType=%d Targets=[%s] Current='%s'"),
        *GetName(), (int32)QueryType,
        *FString::Join(TargetPackageNames, TEXT(", ")),
        *Tracker->GetCurrentLevelPackageName());

    // ── 5. Собственно проверка ───────────────────────────────────────────
    bool bResult = false;

    switch (QueryType)
    {
    case ELocationQueryType::CurrentlyAtTarget:
    {
        const FString Current = Tracker->GetCurrentLevelPackageName();
        bResult = TargetPackageNames.Contains(Current);
        break;
    }

    case ELocationQueryType::HasEnteredEver:
    {
        for (const FString& Pkg : TargetPackageNames)
        {
            if (Tracker->HasEnteredEver(Pkg))
            {
                bResult = true;
                break;
            }
        }
        break;
    }

    case ELocationQueryType::HasEnteredRecently:
    {
        const float Window = FMath::Max(0.0f, TimeWindowMinutes);
        for (const FString& Pkg : TargetPackageNames)
        {
            if (Tracker->HasEnteredInWindow(Pkg, Window))
            {
                bResult = true;
                break;
            }
        }
        break;
    }

    case ELocationQueryType::LeftAndReturned:
    {
        const float Window = FMath::Max(0.0f, TimeWindowMinutes);
        for (const FString& Pkg : TargetPackageNames)
        {
            if (Tracker->HasLeftAndReturned(Pkg, Window))
            {
                bResult = true;
                break;
            }
        }
        break;
    }

    case ELocationQueryType::VisitCountAtLeast:
    {
        int32 Total = 0;
        for (const FString& Pkg : TargetPackageNames)
        {
            Total += Tracker->GetEnterCount(Pkg);
        }
        bResult = (Total >= VisitThreshold);
        break;
    }

    default:
        bResult = false;
        break;
    }

    // ── 6. Итоговый лог ──────────────────────────────────────────────────
    UE_LOG(LogTemp, Warning,
        TEXT("LocationConditionAsset '%s': RESULT = %s"),
        *GetName(), bResult ? TEXT("TRUE") : TEXT("false"));

    return bResult;
}
