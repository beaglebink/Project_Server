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

// ─────────────────────────────────────────────────────────────────────────────
// Индекс сцен → адрес
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::BuildSceneIndex()
{
    SceneToAddress.Empty();
    LocationDisplayNames.Empty();

    auto RegisterDisplayName = [this](ELocationLevel Level, const FGuid& Id, const FText& DisplayName, const FString& FallbackName)
        {
            if (!Id.IsValid()) return;

            FText Label = DisplayName;
            if (Label.IsEmpty())
                Label = FText::FromString(FallbackName);

            LocationDisplayNames.Add(MakeKey(Level, Id), Label);
        };

    // ── Map ──────────────────────────────────────────────────────────────
    {
        TArray<UWorldMapAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("WorldMapAsset"));
        for (UWorldMapAsset* M : All)
        {
            if (!M) continue;
            RegisterDisplayName(ELocationLevel::Map, M->WorldMapID, M->DisplayName, M->GetName());
        }
    }

    // ── Region ───────────────────────────────────────────────────────────
    {
        TArray<UWorldRegionAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("WorldRegionAsset"));
        for (UWorldRegionAsset* R : All)
        {
            if (!R) continue;

            RegisterDisplayName(ELocationLevel::Region, R->WorldRegionID, R->DisplayName, R->GetName());

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
            RegisterDisplayName(ELocationLevel::Street, S->StreetID, S->DisplayName, S->GetName());
        }
    }

    // ── Building ─────────────────────────────────────────────────────────
    {
        TArray<UInteriorSetAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("InteriorSetAsset"));
        for (UInteriorSetAsset* B : All)
        {
            if (!B) continue;
            RegisterDisplayName(ELocationLevel::Building, B->InteriorSetID, B->DisplayName, B->GetName());
        }
    }

    // ── Floor ────────────────────────────────────────────────────────────
    {
        TArray<UFloorAsset*> All;
        LoadAllAssetsOfClass(All, TEXT("FloorAsset"));
        for (UFloorAsset* F : All)
        {
            if (!F) continue;

            RegisterDisplayName(ELocationLevel::Floor, F->FloorID, F->DisplayName, F->GetName());

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
        TEXT("LocationTracker: SceneIndex built — %d scene(s), %d location name(s)."),
        SceneToAddress.Num(), LocationDisplayNames.Num());
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
            }
        }

        if (NewId.IsValid())
        {
            FLocationVisitState& S = FindOrAddState(L, NewId);
            if (S.FirstEnteredAt == FDateTime::MinValue()) S.FirstEnteredAt = Now;
            S.LastEnteredAt = Now;
            S.EnterCount++;
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
    }

    // Хелпер: "DisplayName[Enter/Leave]" или "-" для пустого звена.
    auto FormatLevel = [this](ELocationLevel Level, const FGuid& Id) -> FString
        {
            if (!Id.IsValid()) return TEXT("-");

            const FString Label = GetLocationLabel(Level, Id);

            int32 Enter = 0;
            int32 Leave = 0;

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

    // ── Тестовые логи ────────────────────────────────────────────────────
    UE_LOG(LogTemp, Log,
        TEXT("LocationTracker: Leaving    %s"),
        *FormatAddress(OldAddress));

    CurrentAddress = NewAddress;

    UE_LOG(LogTemp, Log,
        TEXT("LocationTracker: Arrived at %s"),
        *FormatAddress(NewAddress));
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

    const FGuid TargetStreetId = P->TargetStreet->StreetID;
    if (!TargetStreetId.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationTracker: StreetTransition payload has invalid StreetID"));
        return;
    }

    // Собираем новый адрес: игрок на улице — Building/Floor пусты.
    // Регион и карта берутся из иерархии самой улицы, чтобы корректно
    // отработать переход в другой регион/карту.
    FLocationVisitAddress NewAddr;
    NewAddr.StreetId = TargetStreetId;

    if (UWorldRegionAsset* Region = P->TargetStreet->ParentWorldRegion.LoadSynchronous())
    {
        NewAddr.RegionId = Region->WorldRegionID;
        if (UWorldMapAsset* Map = Region->ParentWorldMap.LoadSynchronous())
            NewAddr.MapId = Map->WorldMapID;
    }

    // Идемпотентность: если адрес уже в точности такой — игнорируем.
    // Это отсекает повторные срабатывания при нахождении на улице.
    if (CurrentAddress == NewAddr)
    {
        return;
    }

    ApplyAddressTransition(NewAddr);
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