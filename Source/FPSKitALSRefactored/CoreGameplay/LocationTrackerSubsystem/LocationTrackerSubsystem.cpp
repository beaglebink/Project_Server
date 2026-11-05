#include "LocationTrackerSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "LevelLoadedPayload.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "../ChoreSystem/ChoreManagerSubsystem.h"
#include "LocationVisitResetPayload.h"
#include "LocationStreetTransitionPayload.h"

#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/WorldMapAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "LocationVisitEventPayload.h"
#include "StreetVisitEventPayload.h"
#include "BuildingVisitEventPayload.h"

// ─────────────────────────────────────────────────────────────────────────────
// Утилиты
// ─────────────────────────────────────────────────────────────────────────────
FString ULocationTrackerSubsystem::NormalizeLevelName(const FString& InPath)
{
    if (InPath.IsEmpty()) return FString();

    FString PackagePath = InPath;
    if (PackagePath.Contains(TEXT(".")))
        PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);

    FString Base = FPaths::GetBaseFilename(PackagePath);

    const int32 PIEPos = Base.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
    if (PIEPos == 0)
    {
        int32 Cursor = PIEPos + 7;
        while (Cursor < Base.Len() && FChar::IsDigit(Base[Cursor])) ++Cursor;
        if (Cursor < Base.Len() && Base[Cursor] == TEXT('_')) ++Cursor;
        Base = Base.Mid(Cursor);
    }
    return Base.ToLower();
}

FString ULocationTrackerSubsystem::MakeKey(ELocationLevel Level, const FGuid& Id)
{
    return FString::Printf(TEXT("%d|%s"), (int32)Level, *Id.ToString());
}

static bool MatchesDisplayName(const UObject* Asset, const FText& InDisplayName)
{
    if (!Asset) return false;
    const FString Target = InDisplayName.ToString();
    if (Target.IsEmpty()) return false;

    if (FProperty* Prop = Asset->GetClass()->FindPropertyByName(TEXT("DisplayName")))
    {
        if (const FText* Text = Prop->ContainerPtrToValuePtr<FText>(Asset))
        {
            if (!Text->IsEmpty())
                return Text->ToString().Equals(Target, ESearchCase::CaseSensitive);
        }
    }
    return Asset->GetName().Equals(Target, ESearchCase::CaseSensitive);
}

// У всех ассетов локаций есть поля LocationTag и ClassificationTags,
// но общего базового класса с ними нет. Тянем через рефлексию — так же,
// как MatchesDisplayName выше. Если поле в классе отсутствует (например,
// ассет старого формата) — возвращаем пустые значения.
static void ExtractLocationTagsFromAsset(
    UObject* Asset,
    FGameplayTag& OutLocationTag,
    FGameplayTagContainer& OutClassificationTags)
{
    OutLocationTag = FGameplayTag();
    OutClassificationTags = FGameplayTagContainer();
    if (!Asset) return;

    if (FProperty* Prop = Asset->GetClass()->FindPropertyByName(TEXT("LocationTag")))
    {
        if (const FGameplayTag* Tag = Prop->ContainerPtrToValuePtr<FGameplayTag>(Asset))
            OutLocationTag = *Tag;
    }
    if (FProperty* Prop = Asset->GetClass()->FindPropertyByName(TEXT("ClassificationTags")))
    {
        if (const FGameplayTagContainer* Tags = Prop->ContainerPtrToValuePtr<FGameplayTagContainer>(Asset))
            OutClassificationTags = *Tags;
    }
}

template<typename TAsset>
static void LoadAllAssetsOfClass(TArray<TAsset*>& Out, const TCHAR* ClassName)
{
    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> List;
    ARM.Get().GetAssetsByClass(
        FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), ClassName), List, true);
    for (const FAssetData& AD : List)
        if (TAsset* A = Cast<TAsset>(AD.ToSoftObjectPath().TryLoad()))
            Out.Add(A);
}

// ─────────────────────────────────────────────────────────────────────────────
// Initialize / Deinitialize
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Collection.InitializeDependency<UGameSaveSubsystem>();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
        SaveSys->RegisterSaveableSubsystem(this);

    BuildSceneIndex();

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        LocationEventCondition = NewObject<UOutcomeConditionAsset>(this);
        LocationEventCondition->OperatorType = EConditionOperator::Composite;
        LocationEventCondition->FilterRow.OutcomeType = EOutcomeType::Interior;
        LocationEventCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
        LocationEventCondition->CompileCondition();

        if (LocationEventCondition->GetCondition().IsValid())
        {
            LocationEventHandler = EventBus->RegisterHandler(
                LocationEventCondition,
                FOutcomeHandlerDelegate::CreateUObject(this, &ULocationTrackerSubsystem::HandleLocationEvent));
        }

        LocationResetCondition = NewObject<UOutcomeConditionAsset>(this);
        LocationResetCondition->OperatorType = EConditionOperator::Composite;
        LocationResetCondition->FilterRow.OutcomeType = EOutcomeType::Interior;
        LocationResetCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
        LocationResetCondition->FilterRow.InteriorType = EOutcomeInterior::LocationVisitReset;
        LocationResetCondition->FilterRow.InteriorComparison = EConditionComparison::Equals;
        LocationResetCondition->CompileCondition();

        if (LocationResetCondition->GetCondition().IsValid())
        {
            LocationResetHandler = EventBus->RegisterHandler(
                LocationResetCondition,
                FOutcomeHandlerDelegate::CreateUObject(this, &ULocationTrackerSubsystem::HandleVisitReset));
        }

        StreetTransitionCondition = NewObject<UOutcomeConditionAsset>(this);
        StreetTransitionCondition->OperatorType = EConditionOperator::Composite;
        StreetTransitionCondition->FilterRow.OutcomeType = EOutcomeType::Interior;
        StreetTransitionCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
        StreetTransitionCondition->FilterRow.InteriorType = EOutcomeInterior::StreetTransition;
        StreetTransitionCondition->FilterRow.InteriorComparison = EConditionComparison::Equals;
        StreetTransitionCondition->CompileCondition();

        if (StreetTransitionCondition->GetCondition().IsValid())
        {
            StreetTransitionHandler = EventBus->RegisterHandler(
                StreetTransitionCondition,
                FOutcomeHandlerDelegate::CreateUObject(this, &ULocationTrackerSubsystem::HandleStreetTransition));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("LocationTrackerSubsystem: Initialized. %d scene(s) in index."),
        SceneToAddress.Num());
}

void ULocationTrackerSubsystem::Deinitialize()
{
    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        if (LocationEventHandler.IsValid()) { EventBus->UnregisterHandler(LocationEventHandler);   LocationEventHandler.Invalidate(); }
        if (LocationResetHandler.IsValid()) { EventBus->UnregisterHandler(LocationResetHandler);   LocationResetHandler.Invalidate(); }
        if (StreetTransitionHandler.IsValid()) { EventBus->UnregisterHandler(StreetTransitionHandler);StreetTransitionHandler.Invalidate(); }
    }
    LocationEventCondition = nullptr;
    LocationResetCondition = nullptr;
    StreetTransitionCondition = nullptr;

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    SceneToAddress.Empty();
    VisitHistory.Empty();
    CurrentAddress.Reset();

    Super::Deinitialize();
}

FGameplayTag ULocationTrackerSubsystem::GetCurrentLocation() const
{
    // 1. Основной путь — по уже установленному адресу.
    if (FGameplayTag Tag = FindLocationTagInAddress(CurrentAddress); Tag.IsValid())
        return Tag;

    // 2. Fallback — по имени текущего мира. Пробуем несколько источников
    // имени и нечёткое сопоставление, чтобы покрыть World Partition,
    // стриминговые подуровни и PIE-суффиксы.
    UWorld* World = GetWorld();
    if (!World) return FGameplayTag();

    FLocationVisitAddress Addr;
    FString MatchedKey;
    if (ResolveAddressFromCurrentMap(World, Addr, MatchedKey))
    {
        if (FGameplayTag Tag = FindLocationTagInAddress(Addr); Tag.IsValid())
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("LocationTracker::GetCurrentLocation: fallback matched scene '%s'"), *MatchedKey);
            return Tag;
        }
    }

    // 3. Диагностика — только в аномальном случае. Явно показывает, что
    // не совпало: имя мира, ключи SceneToAddress, или пустые GUID/теги
    // в ассетах.
    {
        const FString MapName = World->GetMapName();
        const FString MapNameNorm = NormalizeLevelName(MapName);

        FString PkgName = TEXT("<null>");
        FString PkgNameNorm;
        if (UPackage* Pkg = World->GetOutermost())
        {
            PkgName = Pkg->GetName();
            PkgNameNorm = NormalizeLevelName(PkgName);
        }

        UE_LOG(LogTemp, Warning,
            TEXT("LocationTracker::GetCurrentLocation: no tag. ")
            TEXT("CurrentAddress.MapId valid=%d, MapName='%s' (norm='%s'), ")
            TEXT("Package='%s' (norm='%s'), SceneToAddress entries=%d."),
            CurrentAddress.MapId.IsValid() ? 1 : 0,
            *MapName, *MapNameNorm,
            *PkgName, *PkgNameNorm,
            SceneToAddress.Num());

        if (SceneToAddress.Num() > 0)
        {
            FString KeysDump;
            for (const auto& Pair : SceneToAddress)
            {
                if (!KeysDump.IsEmpty()) KeysDump += TEXT(", ");
                KeysDump += Pair.Key;
            }
            UE_LOG(LogTemp, Warning,
                TEXT("LocationTracker::GetCurrentLocation: SceneToAddress keys: [%s]"), *KeysDump);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LocationTracker::GetCurrentLocation: SceneToAddress is EMPTY. ")
                TEXT("Проверьте RegionLevel у WorldRegionAsset и FloorLevel у FloorAsset — ")
                TEXT("сцены без указанного Level в индекс не попадают."));
        }
    }

    return FGameplayTag();
}

FGameplayTag ULocationTrackerSubsystem::FindLocationTagInAddress(
    const FLocationVisitAddress& Addr) const
{
    static const ELocationLevel Order[] = {
        ELocationLevel::Floor,
        ELocationLevel::Building,
        ELocationLevel::Street,
        ELocationLevel::Region,
        ELocationLevel::Map
    };

    for (ELocationLevel L : Order)
    {
        const FGuid Id = Addr.Get(L);
        if (!Id.IsValid()) continue;

        UObject* Asset = GetLocationAsset(L, Id);
        if (!Asset)
        {
            UE_LOG(LogTemp, Verbose,
                TEXT("LocationTracker::FindLocationTagInAddress: level %s id=%s — ")
                TEXT("asset not found in LocationAssets. ")
                TEXT("Likely the GUID in the asset is invalid or the asset wasn't loaded."),
                *StaticEnum<ELocationLevel>()->GetValueAsString(L), *Id.ToString());
            continue;
        }

        FGameplayTag Tag;
        FGameplayTagContainer Unused;
        ExtractLocationTagsFromAsset(Asset, Tag, Unused);

        if (Tag.IsValid())
            return Tag;

        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::FindLocationTagInAddress: level %s asset=%s — ")
            TEXT("LocationTag is empty. Fill it in the asset to enable tag-based detection."),
            *StaticEnum<ELocationLevel>()->GetValueAsString(L), *Asset->GetName());
    }

    return FGameplayTag();
}

// ─────────────────────────────────────────────────────────────────────────────
// Индекс сцен → адрес
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::BuildSceneIndex()
{
    SceneToAddress.Empty();
    LocationDisplayNames.Empty();
    LocationAssets.Empty();

    auto RegisterLocation = [this](ELocationLevel Level, const FGuid& Id,
        UObject* Asset, const FText& DisplayName,
        const FString& FallbackName)
        {
            if (!Id.IsValid()) return;

            FText Label = DisplayName;
            if (Label.IsEmpty())
                Label = FText::FromString(FallbackName);

            const FString Key = MakeKey(Level, Id);
            LocationDisplayNames.Add(Key, Label);
            if (Asset)
                LocationAssets.Add(Key, Asset);
        };

    // ── Map ──────────────────────────────────────────────────────────────
    {
        TArray<UWorldMapAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("WorldMapAsset"));
        for (UWorldMapAsset* M : All)
        {
            if (!M) continue;
            RegisterLocation(ELocationLevel::Map, M->WorldMapID, M, M->DisplayName, M->GetName());
        }
    }

    // ── Region ───────────────────────────────────────────────────────────
    {
        TArray<UWorldRegionAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("WorldRegionAsset"));
        for (UWorldRegionAsset* R : All)
        {
            if (!R) continue;

            RegisterLocation(ELocationLevel::Region, R->WorldRegionID, R, R->DisplayName, R->GetName());

            // Также — заполняем SceneToAddress, если у региона есть RegionLevel.
            if (R->RegionLevel.IsNull()) continue;
            const FString Norm = NormalizeLevelName(R->RegionLevel.ToSoftObjectPath().GetLongPackageName());
            if (Norm.IsEmpty()) continue;

            FLocationVisitAddress Addr;
            Addr.RegionId = R->WorldRegionID;
            if (UWorldMapAsset* M = R->ParentWorldMap.LoadSynchronous())
                Addr.MapId = M->WorldMapID;

            if (SceneToAddress.Contains(Norm))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("LocationTracker: scene '%s' already registered (region on existing scene?). Skipping."), *Norm);
                continue;
            }
            SceneToAddress.Add(Norm, Addr);
        }
    }

    // ── Street ───────────────────────────────────────────────────────────
    {
        TArray<UStreetAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("StreetAsset"));
        for (UStreetAsset* S : All)
        {
            if (!S) continue;
            RegisterLocation(ELocationLevel::Street, S->StreetID, S, S->DisplayName, S->GetName());
        }
    }

    // ── Building ─────────────────────────────────────────────────────────
    {
        TArray<UInteriorSetAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("InteriorSetAsset"));
        for (UInteriorSetAsset* B : All)
        {
            if (!B) continue;
            RegisterLocation(ELocationLevel::Building, B->InteriorSetID, B, B->DisplayName, B->GetName());
        }
    }

    // ── Floor ────────────────────────────────────────────────────────────
    {
        TArray<UFloorAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("FloorAsset"));
        for (UFloorAsset* F : All)
        {
            if (!F) continue;

            RegisterLocation(ELocationLevel::Floor, F->FloorID, F, F->DisplayName, F->GetName());

            // Также — заполняем SceneToAddress, если у этажа есть FloorLevel.
            if (F->FloorLevel.IsNull()) continue;
            const FString Norm = NormalizeLevelName(F->FloorLevel.ToSoftObjectPath().GetLongPackageName());
            if (Norm.IsEmpty()) continue;

            FLocationVisitAddress Addr;
            Addr.FloorId = F->FloorID;

            if (UInteriorSetAsset* B = F->ParentInteriorSet.LoadSynchronous())
            {
                Addr.BuildingId = B->InteriorSetID;
                if (UStreetAsset* S = B->ParentStreet.LoadSynchronous())
                {
                    Addr.StreetId = S->StreetID;
                    if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous())
                    {
                        Addr.RegionId = R->WorldRegionID;
                        if (UWorldMapAsset* M = R->ParentWorldMap.LoadSynchronous())
                            Addr.MapId = M->WorldMapID;
                    }
                }
            }

            if (SceneToAddress.Contains(Norm))
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("LocationTracker: scene '%s' already registered (two floors on same level?). Overwriting."), *Norm);
            }
            SceneToAddress.Add(Norm, Addr);
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("LocationTracker: SceneIndex built — %d scene(s), %d location name(s), %d location asset(s)."),
        SceneToAddress.Num(), LocationDisplayNames.Num(), LocationAssets.Num());
}

bool ULocationTrackerSubsystem::FindAddressForScene(const FString& NormScene, FLocationVisitAddress& OutAddr) const
{
    if (const FLocationVisitAddress* Found = SceneToAddress.Find(NormScene))
    {
        OutAddr = *Found;
        return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Обработка LevelLoaded
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::HandleLocationEvent(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType != EOutcomeType::Interior) return;
    if (Outcome.OutcomeInterior != EOutcomeInterior::LevelLoaded) return;

    ULevelLoadedPayload* P = Cast<ULevelLoadedPayload>(Outcome.Payload);
    if (!P) return;

    const FString Norm = NormalizeLevelName(P->LevelPackageName);
    if (Norm.IsEmpty()) return;

    FLocationVisitAddress NewAddr;
    if (!FindAddressForScene(Norm, NewAddr))
    {
        UE_LOG(LogTemp, Verbose, TEXT("LocationTracker: scene '%s' not in index — ignoring"), *Norm);
        return;
    }

    // Перенос Street имеет смысл ТОЛЬКО для региональных сцен.
    if (!NewAddr.BuildingId.IsValid() && NewAddr.RegionId.IsValid())
    {
        if (CurrentAddress.RegionId == NewAddr.RegionId)
            NewAddr.StreetId = CurrentAddress.StreetId;
    }

    ApplyAddressTransition(NewAddr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Применение перехода адреса
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::ApplyAddressTransition(const FLocationVisitAddress& NewAddress)
{
    const FLocationVisitAddress OldAddress = CurrentAddress;

    if (OldAddress == NewAddress) return;

    static const ELocationLevel Levels[] = {
        ELocationLevel::Map, ELocationLevel::Region,
        ELocationLevel::Street, ELocationLevel::Building, ELocationLevel::Floor
    };

    const FDateTime Now = GetGameTimeNow();

    for (ELocationLevel L : Levels)
    {
        const FGuid OldId = OldAddress.Get(L);
        const FGuid NewId = NewAddress.Get(L);
        if (OldId == NewId) continue;

        if (OldId.IsValid())
        {
            // Для Street: если старый адрес уже был внутри дома, то «уход
            // с улицы» был засчитан в момент входа в тот дом (см. спецправило
            // ниже). Второй раз Leave[Street] писать не нужно.
            const bool bSkipLeaveForStreet =
                (L == ELocationLevel::Street && OldAddress.BuildingId.IsValid());

            if (!bSkipLeaveForStreet)
            {
                FLocationVisitState& S = FindOrAddState(L, OldId);
                S.LastLeftAt = Now;
                S.LeaveCount++;
                PublishLocationVisitEvent(L, OldId, /*bEntered=*/false);
            }
        }

        if (NewId.IsValid())
        {
            FLocationVisitState& S = FindOrAddState(L, NewId);
            if (S.FirstEnteredAt == FDateTime::MinValue()) S.FirstEnteredAt = Now;
            S.LastEnteredAt = Now;
            S.EnterCount++;
            PublishLocationVisitEvent(L, NewId, /*bEntered=*/true);
        }
    }

    // Спецправило Street: «оказались в доме на улице S» → Leave[S]++.
    if (NewAddress.BuildingId.IsValid()
        && NewAddress.StreetId.IsValid()
        && (OldAddress.StreetId != NewAddress.StreetId || !OldAddress.BuildingId.IsValid()))
    {
        FLocationVisitState& S = FindOrAddState(ELocationLevel::Street, NewAddress.StreetId);
        S.LastLeftAt = Now;
        S.LeaveCount++;
        PublishLocationVisitEvent(ELocationLevel::Street, NewAddress.StreetId, /*bEntered=*/false);
    }

    // Спецправило Street: «вышли из дома на ту же улицу» → Enter[Street]++.
    if (OldAddress.BuildingId.IsValid()
        && !NewAddress.BuildingId.IsValid()
        && OldAddress.StreetId.IsValid()
        && OldAddress.StreetId == NewAddress.StreetId)
    {
        FLocationVisitState& S = FindOrAddState(ELocationLevel::Street, OldAddress.StreetId);
        if (S.FirstEnteredAt == FDateTime::MinValue()) S.FirstEnteredAt = Now;
        S.LastEnteredAt = Now;
        S.EnterCount++;
        PublishLocationVisitEvent(ELocationLevel::Street, OldAddress.StreetId, /*bEntered=*/true);
    }

    // Хелпер: "DisplayName[Enter/Leave]" или "-" для пустого звена.
    auto FormatLevel = [this](ELocationLevel Level, const FGuid& Id) -> FString
        {
            if (!Id.IsValid()) return TEXT("-");
            const FString Label = GetLocationLabel(Level, Id);
            int32 Enter = 0, Leave = 0;
            const FString Key = MakeKey(Level, Id);
            if (const FLocationVisitState* S = VisitHistory.Find(Key))
            {
                Enter = S->EnterCount;
                Leave = S->LeaveCount;
            }
            return FString::Printf(TEXT("%s[%d/%d]"), *Label, Enter, Leave);
        };

    auto FormatAddress = [&](const FLocationVisitAddress& Addr) -> FString
        {
            return FString::Printf(TEXT("(M=%s, R=%s, S=%s, B=%s, F=%s)"),
                *FormatLevel(ELocationLevel::Map, Addr.MapId),
                *FormatLevel(ELocationLevel::Region, Addr.RegionId),
                *FormatLevel(ELocationLevel::Street, Addr.StreetId),
                *FormatLevel(ELocationLevel::Building, Addr.BuildingId),
                *FormatLevel(ELocationLevel::Floor, Addr.FloorId));
        };

    UE_LOG(LogTemp, Log, TEXT("LocationTracker: Leaving    %s"), *FormatAddress(OldAddress));

    CurrentAddress = NewAddress;

    UE_LOG(LogTemp, Log, TEXT("LocationTracker: Arrived at %s"), *FormatAddress(NewAddress));
}

void ULocationTrackerSubsystem::PublishLocationVisitEvent(
    ELocationLevel Level, const FGuid& Id, bool bEntered)
{
    if (!Id.IsValid()) return;
    if (Level == ELocationLevel::Default) return;

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    UEventBusSubsystem* EventBus = GI->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    const FString Key = MakeKey(Level, Id);

    UObject* Asset = nullptr;
    if (const TObjectPtr<UObject>* Found = LocationAssets.Find(Key))
        Asset = Found->Get();

    FText DisplayName;
    if (const FText* Found = LocationDisplayNames.Find(Key))
        DisplayName = *Found;

    // Тянем теги из ассета один раз — пригодятся и общему payload,
    // и специфичному (Street / Building).
    FGameplayTag LocationTag;
    FGameplayTagContainer ClassificationTags;
    ExtractLocationTagsFromAsset(Asset, LocationTag, ClassificationTags);

    // ── Общее событие LocationEntered / LocationLeft ───────────────────
    // Публикуется для ВСЕХ уровней — Map, Region, Street, Building, Floor.
    // Подписчику, которому нужен только один уровень, проще слушать
    // специфичное событие (StreetEntered / BuildingEntered).
    {
        ULocationVisitEventPayload* Payload =
            EventBus->CreatePayload<ULocationVisitEventPayload>();
        if (Payload)
        {
            Payload->Setup(Level, Id, Asset, DisplayName, LocationTag, ClassificationTags);

            FOutcomeEventBase Event;
            Event.OutcomeType = EOutcomeType::Interior;
            Event.OutcomeInterior = bEntered
                ? EOutcomeInterior::LocationEntered
                : EOutcomeInterior::LocationLeft;
            Event.Payload = Payload;
            EventBus->PublishOutcome(Event);
        }
    }

    // ── Специфичное событие для улицы ──────────────────────────────────
    if (Level == ELocationLevel::Street)
    {
        UStreetVisitEventPayload* Payload =
            EventBus->CreatePayload<UStreetVisitEventPayload>();
        if (Payload)
        {
            Payload->Setup(Cast<UStreetAsset>(Asset), Id, DisplayName,
                LocationTag, ClassificationTags);

            FOutcomeEventBase Event;
            Event.OutcomeType = EOutcomeType::Interior;
            Event.OutcomeInterior = bEntered
                ? EOutcomeInterior::StreetEntered
                : EOutcomeInterior::StreetLeft;
            Event.Payload = Payload;
            EventBus->PublishOutcome(Event);
        }
    }
    // ── Специфичное событие для дома ───────────────────────────────────
    else if (Level == ELocationLevel::Building)
    {
        UBuildingVisitEventPayload* Payload =
            EventBus->CreatePayload<UBuildingVisitEventPayload>();
        if (Payload)
        {
            Payload->Setup(Cast<UInteriorSetAsset>(Asset), Id, DisplayName,
                LocationTag, ClassificationTags);

            FOutcomeEventBase Event;
            Event.OutcomeType = EOutcomeType::Interior;
            Event.OutcomeInterior = bEntered
                ? EOutcomeInterior::BuildingEntered
                : EOutcomeInterior::BuildingLeft;
            Event.Payload = Payload;
            EventBus->PublishOutcome(Event);
        }
    }
}

FLocationVisitState& ULocationTrackerSubsystem::FindOrAddState(ELocationLevel Level, const FGuid& Id)
{
    const FString Key = MakeKey(Level, Id);
    FLocationVisitState& S = VisitHistory.FindOrAdd(Key);
    if (!S.LocationId.IsValid())
    {
        S.Level = Level;
        S.LocationId = Id;

        if (const FText* Found = LocationDisplayNames.Find(Key))
            S.DisplayName = *Found;
    }
    return S;
}

// ─────────────────────────────────────────────────────────────────────────────
// Публичные запросы
// ─────────────────────────────────────────────────────────────────────────────
const FLocationVisitState* ULocationTrackerSubsystem::FindVisitState(const FLocationVisitKey& Key) const
{
    if (!Key.IsValid()) return nullptr;
    return VisitHistory.Find(MakeKey(Key.Level, Key.LocationId));
}

bool ULocationTrackerSubsystem::HasEnteredEver(const FLocationVisitKey& Key) const
{
    const FLocationVisitState* S = FindVisitState(Key);
    return S && S->LastEnteredAt != FDateTime::MinValue();
}

bool ULocationTrackerSubsystem::HasLeftAndReturned(const FLocationVisitKey& Key) const
{
    const FLocationVisitState* S = FindVisitState(Key);
    if (!S) return false;
    if (S->EnterCount < 1) return false;
    if (S->LastLeftAt == FDateTime::MinValue() || S->LastEnteredAt == FDateTime::MinValue()) return false;
    return S->LastEnteredAt > S->LastLeftAt;
}

int32 ULocationTrackerSubsystem::GetEnterCount(const FLocationVisitKey& Key) const
{
    const FLocationVisitState* S = FindVisitState(Key);
    return S ? S->EnterCount : 0;
}

int32 ULocationTrackerSubsystem::GetLeaveCount(const FLocationVisitKey& Key) const
{
    const FLocationVisitState* S = FindVisitState(Key);
    return S ? S->LeaveCount : 0;
}

FDateTime ULocationTrackerSubsystem::GetGameTimeNow() const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UChoreManagerSubsystem* CM = GI->GetSubsystem<UChoreManagerSubsystem>())
            return CM->GetGameTime();
    }
    return FDateTime::UtcNow();
}

// ─────────────────────────────────────────────────────────────────────────────
// Резолвер DisplayName → ключ
// ─────────────────────────────────────────────────────────────────────────────
bool ULocationTrackerSubsystem::ResolveLocationKeyByDisplayName(
    const FText& DisplayName,
    FLocationVisitKey& OutKey,
    ELocationLevel PreferredLevel)
{
    OutKey = FLocationVisitKey();
    if (DisplayName.IsEmpty()) return false;

    int32 MatchCount = 0;

    auto TryAsset = [&](UObject* Asset, ELocationLevel Level, const FGuid& Id)
        {
            if (!MatchesDisplayName(Asset, DisplayName)) return;
            ++MatchCount;
            if (MatchCount == 1)
            {
                OutKey.Level = Level;
                OutKey.LocationId = Id;
            }
        };

    auto ScanFloors = [&]()
        {
            TArray<UFloorAsset*> All; LoadAllAssetsOfClass(All, TEXT("FloorAsset"));
            for (UFloorAsset* A : All) TryAsset(A, ELocationLevel::Floor, A ? A->FloorID : FGuid());
        };
    auto ScanBuildings = [&]()
        {
            TArray<UInteriorSetAsset*> All; LoadAllAssetsOfClass(All, TEXT("InteriorSetAsset"));
            for (UInteriorSetAsset* A : All) TryAsset(A, ELocationLevel::Building, A ? A->InteriorSetID : FGuid());
        };
    auto ScanStreets = [&]()
        {
            TArray<UStreetAsset*> All; LoadAllAssetsOfClass(All, TEXT("StreetAsset"));
            for (UStreetAsset* A : All) TryAsset(A, ELocationLevel::Street, A ? A->StreetID : FGuid());
        };
    auto ScanRegions = [&]()
        {
            TArray<UWorldRegionAsset*> All; LoadAllAssetsOfClass(All, TEXT("WorldRegionAsset"));
            for (UWorldRegionAsset* A : All) TryAsset(A, ELocationLevel::Region, A ? A->WorldRegionID : FGuid());
        };
    auto ScanMaps = [&]()
        {
            TArray<UWorldMapAsset*> All; LoadAllAssetsOfClass(All, TEXT("WorldMapAsset"));
            for (UWorldMapAsset* A : All) TryAsset(A, ELocationLevel::Map, A ? A->WorldMapID : FGuid());
        };

    if (PreferredLevel == ELocationLevel::Default)
    {
        ScanFloors();
        ScanBuildings();
        ScanStreets();
        ScanRegions();
        ScanMaps();
    }
    else
    {
        switch (PreferredLevel)
        {
        case ELocationLevel::Floor:    ScanFloors();    break;
        case ELocationLevel::Building: ScanBuildings(); break;
        case ELocationLevel::Street:   ScanStreets();   break;
        case ELocationLevel::Region:   ScanRegions();   break;
        case ELocationLevel::Map:      ScanMaps();      break;
        default: break;
        }
    }

    if (MatchCount == 0)
    {
        if (PreferredLevel == ELocationLevel::Default)
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LocationTracker: no location asset with DisplayName '%s'"),
                *DisplayName.ToString());
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LocationTracker: no location asset with DisplayName '%s' at level %s"),
                *DisplayName.ToString(),
                *StaticEnum<ELocationLevel>()->GetValueAsString(PreferredLevel));
        }
        return false;
    }
    if (MatchCount > 1)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationTracker: DisplayName '%s' matched %d assets, using first"),
            *DisplayName.ToString(), MatchCount);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// HandleVisitReset
// ─────────────────────────────────────────────────────────────────────────────
static void CollectKeysUnderFloor(UFloorAsset* F, TArray<FLocationVisitKey>& Out)
{
    if (!F) return;
    Out.Add({ ELocationLevel::Floor, F->FloorID });
}

static void CollectKeysUnderBuilding(UInteriorSetAsset* B, TArray<FLocationVisitKey>& Out)
{
    if (!B) return;
    Out.Add({ ELocationLevel::Building, B->InteriorSetID });
    for (auto& R : B->Floors) CollectKeysUnderFloor(R.LoadSynchronous(), Out);
}

static void CollectKeysUnderStreet(UStreetAsset* S, TArray<FLocationVisitKey>& Out)
{
    if (!S) return;
    Out.Add({ ELocationLevel::Street, S->StreetID });
    for (auto& R : S->InteriorSets) CollectKeysUnderBuilding(R.LoadSynchronous(), Out);
}

static void CollectKeysUnderRegion(UWorldRegionAsset* R, TArray<FLocationVisitKey>& Out)
{
    if (!R) return;
    Out.Add({ ELocationLevel::Region, R->WorldRegionID });
    for (auto& S : R->Streets) CollectKeysUnderStreet(S.LoadSynchronous(), Out);
}

static void CollectKeysUnderMap(UWorldMapAsset* M, TArray<FLocationVisitKey>& Out)
{
    if (!M) return;
    Out.Add({ ELocationLevel::Map, M->WorldMapID });
    for (auto& R : M->Regions) CollectKeysUnderRegion(R.LoadSynchronous(), Out);
}

void ULocationTrackerSubsystem::HandleVisitReset(const FOutcomeEventBase& Outcome)
{
    ULocationVisitResetPayload* P = Cast<ULocationVisitResetPayload>(Outcome.Payload);
    if (!P) return;

    if (P->bResetAll)
    {
        const int32 N = VisitHistory.Num();
        VisitHistory.Empty();
        UE_LOG(LogTemp, Log, TEXT("LocationTracker: Reset ALL visits (%d entries)"), N);
        return;
    }

    TArray<FLocationVisitKey> Keys;
    if (P->TargetFloor)         CollectKeysUnderFloor(P->TargetFloor, Keys);
    else if (P->TargetBuilding) CollectKeysUnderBuilding(P->TargetBuilding, Keys);
    else if (P->TargetStreet)   CollectKeysUnderStreet(P->TargetStreet, Keys);
    else if (P->TargetRegion)   CollectKeysUnderRegion(P->TargetRegion, Keys);
    else if (P->TargetMap)      CollectKeysUnderMap(P->TargetMap, Keys);

    int32 Removed = 0;
    for (const FLocationVisitKey& K : Keys)
        Removed += VisitHistory.Remove(MakeKey(K.Level, K.LocationId));

    UE_LOG(LogTemp, Log,
        TEXT("LocationTracker: Reset visits under hierarchy — %d key(s) processed, %d entries removed"),
        Keys.Num(), Removed);
}

// ─────────────────────────────────────────────────────────────────────────────
// HandleStreetTransition — команда «игрок перешёл на улицу»
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::HandleStreetTransition(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType != EOutcomeType::Interior) return;
    if (Outcome.OutcomeInterior != EOutcomeInterior::StreetTransition) return;

    ULocationStreetTransitionPayload* P = Cast<ULocationStreetTransitionPayload>(Outcome.Payload);
    if (!P || !P->TargetStreet) return;

    // Собираем новый адрес.
    FLocationVisitAddress NewAddr;

    if (P->bClearStreet)
    {
        // Режим «покинул улицу»: StreetId оставляем пустым, Region/Map
        // берём из TargetStreet (родитель улицы). Если у улицы не задан
        // ParentWorldRegion — сохраняем текущие Region/Map из адреса,
        // чтобы не потерять регион.
        if (UWorldRegionAsset* Region = P->TargetStreet->ParentWorldRegion.LoadSynchronous())
        {
            NewAddr.RegionId = Region->WorldRegionID;
            if (UWorldMapAsset* Map = Region->ParentWorldMap.LoadSynchronous())
                NewAddr.MapId = Map->WorldMapID;
        }
        else
        {
            NewAddr.RegionId = CurrentAddress.RegionId;
            NewAddr.MapId = CurrentAddress.MapId;
        }
        // NewAddr.StreetId, BuildingId, FloorId — пустые.
    }
    else
    {
        const FGuid TargetStreetId = P->TargetStreet->StreetID;
        if (!TargetStreetId.IsValid())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LocationTracker: StreetTransition payload has invalid StreetID"));
            return;
        }

        NewAddr.StreetId = TargetStreetId;

        if (UWorldRegionAsset* Region = P->TargetStreet->ParentWorldRegion.LoadSynchronous())
        {
            NewAddr.RegionId = Region->WorldRegionID;
            if (UWorldMapAsset* Map = Region->ParentWorldMap.LoadSynchronous())
                NewAddr.MapId = Map->WorldMapID;
        }
    }

    // Идемпотентность: если адрес уже в точности такой — игнорируем.
    if (CurrentAddress == NewAddr)
    {
        return;
    }

    ApplyAddressTransition(NewAddr);
}

bool ULocationTrackerSubsystem::ResolveAddressFromCurrentMap(
    UWorld* World,
    FLocationVisitAddress& OutAddr,
    FString& OutMatchedKey) const
{
    OutAddr = FLocationVisitAddress();
    OutMatchedKey.Reset();
    if (!World) return false;

    // Собираем все возможные нормализованные имена текущего мира.
    // В разных конфигурациях (PIE, WP, streaming, packaged) GetMapName
    // и путь пакета могут различаться — пробуем оба плюс GetPathName.
    TArray<FString> Candidates;
    auto AddCandidate = [&](const FString& Raw)
        {
            const FString Norm = NormalizeLevelName(Raw);
            if (!Norm.IsEmpty() && !Candidates.Contains(Norm))
                Candidates.Add(Norm);
        };

    AddCandidate(World->GetMapName());
    if (UPackage* Pkg = World->GetOutermost())
        AddCandidate(Pkg->GetName());
    AddCandidate(World->GetPathName());

    // 1. Точное совпадение — самый быстрый и надёжный путь.
    for (const FString& Norm : Candidates)
    {
        if (const FLocationVisitAddress* Found = SceneToAddress.Find(Norm))
        {
            OutAddr = *Found;
            OutMatchedKey = Norm;
            return true;
        }
    }

    // 2. Нечёткое — на случай WP/стриминга, когда имя persistent-уровня
    // не совпадает с именем зарегистрированной сцены. Ищем сцену, чей
    // ключ содержит кандидат или наоборот.
    //
    // Примеры, которые это покрывает:
    //   • GetMapName() = "PortRegion_Persistent", ключ = "portregion";
    //   • GetMapName() = "PortRegion", ключ = "portregion_persistent".
    for (const FString& Norm : Candidates)
    {
        for (const auto& Pair : SceneToAddress)
        {
            const FString& Key = Pair.Key;
            if (Key.Contains(Norm) || Norm.Contains(Key))
            {
                OutAddr = Pair.Value;
                OutMatchedKey = Key;
                return true;
            }
        }
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Save / Load
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    TSharedPtr<FJsonObject> AddrObj = MakeShared<FJsonObject>();
    AddrObj->SetStringField(TEXT("MapId"), CurrentAddress.MapId.ToString());
    AddrObj->SetStringField(TEXT("RegionId"), CurrentAddress.RegionId.ToString());
    AddrObj->SetStringField(TEXT("StreetId"), CurrentAddress.StreetId.ToString());
    AddrObj->SetStringField(TEXT("BuildingId"), CurrentAddress.BuildingId.ToString());
    AddrObj->SetStringField(TEXT("FloorId"), CurrentAddress.FloorId.ToString());
    Root->SetObjectField(TEXT("CurrentAddress"), AddrObj);

    TArray<TSharedPtr<FJsonValue>> VisitsArray;
    for (const auto& Pair : VisitHistory)
    {
        const FLocationVisitState& S = Pair.Value;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetNumberField(TEXT("Level"), (int32)S.Level);
        Obj->SetStringField(TEXT("LocationId"), S.LocationId.ToString());
        Obj->SetStringField(TEXT("FirstEnteredAt"), S.FirstEnteredAt.ToIso8601());
        Obj->SetStringField(TEXT("LastEnteredAt"), S.LastEnteredAt.ToIso8601());
        Obj->SetStringField(TEXT("LastLeftAt"), S.LastLeftAt.ToIso8601());
        Obj->SetNumberField(TEXT("EnterCount"), S.EnterCount);
        Obj->SetNumberField(TEXT("LeaveCount"), S.LeaveCount);
        VisitsArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("Visits"), VisitsArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void ULocationTrackerSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bLoadComplete = false;

    if (InData.SerializedData.IsEmpty())
    {
        bLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        bLoadComplete = true;
        return;
    }

    VisitHistory.Empty();
    CurrentAddress.Reset();

    const TSharedPtr<FJsonObject>* AddrPtr = nullptr;
    if (Root->TryGetObjectField(TEXT("CurrentAddress"), AddrPtr))
    {
        FString S;
        if ((*AddrPtr)->TryGetStringField(TEXT("MapId"), S))      FGuid::Parse(S, CurrentAddress.MapId);
        if ((*AddrPtr)->TryGetStringField(TEXT("RegionId"), S))   FGuid::Parse(S, CurrentAddress.RegionId);
        if ((*AddrPtr)->TryGetStringField(TEXT("StreetId"), S))   FGuid::Parse(S, CurrentAddress.StreetId);
        if ((*AddrPtr)->TryGetStringField(TEXT("BuildingId"), S)) FGuid::Parse(S, CurrentAddress.BuildingId);
        if ((*AddrPtr)->TryGetStringField(TEXT("FloorId"), S))    FGuid::Parse(S, CurrentAddress.FloorId);
    }

    const TArray<TSharedPtr<FJsonValue>>* VisitsArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Visits"), VisitsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *VisitsArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FLocationVisitState S;
            int32 LevelInt = 0;
            Obj->TryGetNumberField(TEXT("Level"), LevelInt);
            S.Level = (ELocationLevel)LevelInt;

            FString IdStr;
            if (Obj->TryGetStringField(TEXT("LocationId"), IdStr)) FGuid::Parse(IdStr, S.LocationId);
            if (!S.LocationId.IsValid()) continue;

            FString Tmp;
            if (Obj->TryGetStringField(TEXT("FirstEnteredAt"), Tmp) && !FDateTime::ParseIso8601(*Tmp, S.FirstEnteredAt))
                S.FirstEnteredAt = FDateTime::MinValue();
            if (Obj->TryGetStringField(TEXT("LastEnteredAt"), Tmp) && !FDateTime::ParseIso8601(*Tmp, S.LastEnteredAt))
                S.LastEnteredAt = FDateTime::MinValue();
            if (Obj->TryGetStringField(TEXT("LastLeftAt"), Tmp) && !FDateTime::ParseIso8601(*Tmp, S.LastLeftAt))
                S.LastLeftAt = FDateTime::MinValue();

            Obj->TryGetNumberField(TEXT("EnterCount"), S.EnterCount);
            Obj->TryGetNumberField(TEXT("LeaveCount"), S.LeaveCount);

            VisitHistory.Add(MakeKey(S.Level, S.LocationId), S);
        }
    }

    bLoadComplete = true;
}

bool ULocationTrackerSubsystem::GetLocationCountsByDisplayName(
    const FText& DisplayName,
    int32& OutEnterCount,
    int32& OutLeaveCount) const
{
    OutEnterCount = 0;
    OutLeaveCount = 0;

    FLocationVisitKey Key;
    if (!ResolveLocationKeyByDisplayName(DisplayName, Key))
        return false;

    const FLocationVisitState* S = FindVisitState(Key);
    if (!S)
    {
        return true;
    }

    OutEnterCount = S->EnterCount;
    OutLeaveCount = S->LeaveCount;
    return true;
}

ULocationVisitEventPayload* ULocationTrackerSubsystem::GetCurrentLocationPayload(
    ELocationLevel Level) const
{
    if (Level == ELocationLevel::Default) return nullptr;

    const FGuid Id = CurrentAddress.Get(Level);
    if (!Id.IsValid()) return nullptr;

    const FString Key = MakeKey(Level, Id);

    UObject* Asset = nullptr;
    if (const TObjectPtr<UObject>* Found = LocationAssets.Find(Key))
        Asset = Found->Get();

    FText DisplayName;
    if (const FText* Found = LocationDisplayNames.Find(Key))
        DisplayName = *Found;

    FGameplayTag LocationTag;
    FGameplayTagContainer ClassificationTags;
    ExtractLocationTagsFromAsset(Asset, LocationTag, ClassificationTags);

    // Outer = подсистема: объект умрёт вместе с ней. Вызывающий держит
    // ссылку (UPROPERTY в BP / TObjectPtr в C++), поэтому до этого момента
    // объект гарантированно жив.
    ULocationVisitEventPayload* Payload =
        NewObject<ULocationVisitEventPayload>(const_cast<ULocationTrackerSubsystem*>(this));
    Payload->Setup(Level, Id, Asset, DisplayName, LocationTag, ClassificationTags);
    return Payload;
}

FString ULocationTrackerSubsystem::GetLocationLabel(ELocationLevel Level, const FGuid& Id) const
{
    if (!Id.IsValid()) return TEXT("-");

    const FString Key = MakeKey(Level, Id);
    if (const FText* Found = LocationDisplayNames.Find(Key))
    {
        if (!Found->IsEmpty())
            return Found->ToString();
    }

    const FString LevelStr = StaticEnum<ELocationLevel>()->GetValueAsString(Level);
    return FString::Printf(TEXT("<%s:%s>"),
        *LevelStr.Left(3),
        *Id.ToString().Left(8));
}

UObject* ULocationTrackerSubsystem::GetLocationAsset(ELocationLevel Level, const FGuid& Id) const
{
    if (!Id.IsValid()) return nullptr;

    const FString Key = MakeKey(Level, Id);
    if (const TObjectPtr<UObject>* Found = LocationAssets.Find(Key))
        return Found->Get();
    return nullptr;
}

UObject* ULocationTrackerSubsystem::GetCurrentLocationAsset(ELocationLevel Level) const
{
    return GetLocationAsset(Level, CurrentAddress.Get(Level));
}

FText ULocationTrackerSubsystem::GetCurrentLocationDisplayName(ELocationLevel Level) const
{
    const FGuid Id = CurrentAddress.Get(Level);
    if (!Id.IsValid()) return FText::GetEmpty();

    const FString Key = MakeKey(Level, Id);
    if (const FText* Found = LocationDisplayNames.Find(Key))
        return *Found;
    return FText::GetEmpty();
}

ELocationLevel ULocationTrackerSubsystem::GetCurrentDeepestLevel() const
{
    if (CurrentAddress.FloorId.IsValid())    return ELocationLevel::Floor;
    if (CurrentAddress.BuildingId.IsValid()) return ELocationLevel::Building;
    if (CurrentAddress.StreetId.IsValid())   return ELocationLevel::Street;
    if (CurrentAddress.RegionId.IsValid())   return ELocationLevel::Region;
    if (CurrentAddress.MapId.IsValid())      return ELocationLevel::Map;
    return ELocationLevel::Default;
}

UObject* ULocationTrackerSubsystem::GetCurrentLocationAssetDeepest() const
{
    return GetCurrentLocationAsset(GetCurrentDeepestLevel());
}

FText ULocationTrackerSubsystem::GetCurrentLocationDisplayNameDeepest() const
{
    return GetCurrentLocationDisplayName(GetCurrentDeepestLevel());
}

FString ULocationTrackerSubsystem::GetCurrentLocationDebugString() const
{
    auto NameAt = [this](ELocationLevel Level) -> FString
        {
            const FGuid Id = CurrentAddress.Get(Level);
            if (!Id.IsValid()) return FString();

            const FString Key = MakeKey(Level, Id);
            if (const FText* Found = LocationDisplayNames.Find(Key))
            {
                if (!Found->IsEmpty())
                    return Found->ToString();
            }
            return Id.ToString().Left(8);
        };

    TArray<FString> Parts;
    for (ELocationLevel L : { ELocationLevel::Map, ELocationLevel::Region,
        ELocationLevel::Street, ELocationLevel::Building,
        ELocationLevel::Floor })
    {
        const FString Name = NameAt(L);
        if (!Name.IsEmpty())
            Parts.Add(Name);
    }

    if (Parts.Num() == 0)
        return TEXT("<nowhere>");

    return FString::Join(Parts, TEXT(" / "));
}

