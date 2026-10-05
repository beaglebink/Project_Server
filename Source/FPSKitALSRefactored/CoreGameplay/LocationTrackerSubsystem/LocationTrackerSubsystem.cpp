#include "LocationTrackerSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "LevelLoadedPayload.h"
#include "InteriorTransitionPayload.h"
#include "../SaveGame/GameSaveSubsystem.h"
#include "../ChoreSystem/ChoreManagerSubsystem.h"
#include "LocationVisitResetPayload.h"

#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/WorldMapAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"

// ─────────────────────────────────────────────────────────────────────────────
FString ULocationTrackerSubsystem::NormalizeLevelName(const FString& InPath)
{
    if (InPath.IsEmpty()) return FString();

    FString PackagePath = InPath;
    if (PackagePath.Contains(TEXT(".")))
        PackagePath = FPackageName::ObjectPathToPackageName(PackagePath);

    FString Base = FPaths::GetBaseFilename(PackagePath);

    // Срезаем ТОЛЬКО PIE-префикс вида "UEDPIE_<n>_".
    // Всё остальное — часть авторского имени карты, её терять нельзя,
    // иначе runtime и ассет дадут разные ключи.
    const int32 PIEPos = Base.Find(TEXT("UEDPIE_"), ESearchCase::IgnoreCase);
    if (PIEPos == 0)
    {
        int32 Cursor = PIEPos + 7;  // после "UEDPIE_"
        while (Cursor < Base.Len() && FChar::IsDigit(Base[Cursor]))
            ++Cursor;
        if (Cursor < Base.Len() && Base[Cursor] == TEXT('_'))
            ++Cursor;
        Base = Base.Mid(Cursor);
    }

    return Base.ToLower();
}

// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Collection.InitializeDependency<UGameSaveSubsystem>();

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        // ── Широкий handler на все Interior-события ──────────────────────
        // Фильтруем по OutcomeType == Interior; конкретный InteriorType
        // проверяется уже внутри HandleLocationEvent (LevelLoaded / FloorLeaving).
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

        // ── Отдельный handler на команду LocationVisitReset ──────────────
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
    }

    // Первый LevelLoaded после старта сессии не считается «входом» игрока —
    // это восстановление мира.
    bSuppressNextLevelLoad = true;

    // Попытка получить стартовый мир, если он уже готов к этому моменту.
    // GetWorld() в GameInstanceSubsystem обычно возвращает мир через GameInstance,
    // и на момент Initialize он уже создан. Если nullptr — оставляем кэш пустым,
    // тогда первый LevelLoaded отработает как '<initial>'.
    if (UWorld* W = GetWorld())
    {
        CachedCurrentLevelPackageName = NormalizeLevelName(W->GetOutermost()->GetName());
    }

    UE_LOG(LogTemp, Log, TEXT("LocationTrackerSubsystem: Initialized."));
}

void ULocationTrackerSubsystem::Deinitialize()
{
    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        if (LocationEventHandler.IsValid())
        {
            EventBus->UnregisterHandler(LocationEventHandler);
            LocationEventHandler.Invalidate();
        }

        if (LocationResetHandler.IsValid())
        {
            EventBus->UnregisterHandler(LocationResetHandler);
            LocationResetHandler.Invalidate();
        }
    }

    LocationEventCondition = nullptr;
    LocationResetCondition = nullptr;

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    VisitHistory.Empty();
    CachedCurrentLevelPackageName.Empty();

    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
FDateTime ULocationTrackerSubsystem::GetGameTimeNow() const
{
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UChoreManagerSubsystem* CM = GI->GetSubsystem<UChoreManagerSubsystem>())
        {
            return CM->GetGameTime();
        }
    }
    return FDateTime::UtcNow();
}

// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::HandleLocationEvent(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType != EOutcomeType::Interior) return;

    // ── LevelLoaded ──────────────────────────────────────────────────────
    if (Outcome.OutcomeInterior == EOutcomeInterior::LevelLoaded)
    {
        ULevelLoadedPayload* P = Cast<ULevelLoadedPayload>(Outcome.Payload);
        if (!P)
        {
            UE_LOG(LogTemp, Warning, TEXT("LocationTracker: LevelLoaded with no payload"));
            return;
        }

        const FString Norm = NormalizeLevelName(P->LevelPackageName);
        if (Norm.IsEmpty()) return;

        // «Откуда пришли» — предыдущий кэш, ещё не перезаписанный.
        const FString PrevLevel = CachedCurrentLevelPackageName;

        // Обновляем кэш текущего уровня — источник правды для FloorLeaving.
        CachedCurrentLevelPackageName = Norm;

        FLocationVisitState& State = VisitHistory.FindOrAdd(Norm);

        State.LevelPackageName = Norm;

        const FDateTime Now = GetGameTimeNow();
        if (State.FirstEnteredAt == FDateTime::MinValue())
            State.FirstEnteredAt = Now;
        State.LastEnteredAt = Now;
        State.EnterCount++;

        UE_LOG(LogTemp, Log,
            TEXT("LocationTracker: LevelLoaded: '%s' -> '%s' [Enter=%d Leave=%d]"),
            PrevLevel.IsEmpty() ? TEXT("<initial>") : *PrevLevel,
            *Norm,
            State.EnterCount, State.LeaveCount);

        /*
        UE_LOG(LogTemp, Log,
            TEXT("LocationTracker: LevelLoaded: '%s' -> '%s' [Enter=%d Leave=%d LastEntered=%s LastLeft=%s]"),
            PrevLevel.IsEmpty() ? TEXT("<initial>") : *PrevLevel,
            *Norm,
            State.EnterCount, State.LeaveCount,
            *State.LastEnteredAt.ToIso8601(),
            State.LastLeftAt == FDateTime::MinValue() ? TEXT("<none>") : *State.LastLeftAt.ToIso8601());
        */
        return;
    }

    // ── FloorLeaving ─────────────────────────────────────────────────────
    if (Outcome.OutcomeInterior == EOutcomeInterior::FloorLeaving)
    {
        // Источник — последний известный уровень из LevelLoaded
        // (или восстановленный из сейва / полученный в Initialize).
        const FString SourceNorm = CachedCurrentLevelPackageName;
        if (SourceNorm.IsEmpty())
        {
            UE_LOG(LogTemp, Warning,
                TEXT("LocationTracker: FloorLeaving but CachedCurrentLevelPackageName is empty (no LevelLoaded yet?)"));
            return;
        }

        // Назначение — из payload, который публикует InteriorSubsystem.
        FString DestDesc = TEXT("<unknown>");
        if (UInteriorTransitionPayload* TP = Cast<UInteriorTransitionPayload>(Outcome.Payload))
        {
            const FString DestPkg = TP->GetTargetLevelPackageName();
            if (!DestPkg.IsEmpty())
            {
                const FString DestNorm = NormalizeLevelName(DestPkg);
                if (!DestNorm.IsEmpty())
                    DestDesc = FString::Printf(TEXT("'%s'"), *DestNorm);
            }
        }

        FLocationVisitState& State = VisitHistory.FindOrAdd(SourceNorm);

        State.LevelPackageName = SourceNorm;
        State.LastLeftAt = GetGameTimeNow();
        State.LeaveCount++;

        UE_LOG(LogTemp, Log,
            TEXT("LocationTracker: FloorLeaving: from='%s' [Enter=%d Leave=%d ] -> to=%s"),
            *SourceNorm,
            State.EnterCount, State.LeaveCount,
            *DestDesc);
        /*
        UE_LOG(LogTemp, Log,
            TEXT("LocationTracker: FloorLeaving: from='%s' [Enter=%d Leave=%d LastEntered=%s LastLeft=%s] -> to=%s"),
            *SourceNorm,
            State.EnterCount, State.LeaveCount,
            State.LastEnteredAt == FDateTime::MinValue() ? TEXT("<none>") : *State.LastEnteredAt.ToIso8601(),
            *State.LastLeftAt.ToIso8601(),
            *DestDesc);
        */
    }
}

// ─────────────────────────────────────────────────────────────────────────────
FString ULocationTrackerSubsystem::GetCurrentLevelPackageName() const
{
    UWorld* World = GetWorld();
    if (!World) return FString();
    return NormalizeLevelName(World->GetOutermost()->GetName());
}

const FLocationVisitState* ULocationTrackerSubsystem::FindVisitState(const FString& NormalizedPackageName) const
{
    if (NormalizedPackageName.IsEmpty()) return nullptr;
    return VisitHistory.Find(NormalizedPackageName);
}

// ─────────────────────────────────────────────────────────────────────────────
bool ULocationTrackerSubsystem::HasEnteredEver(const FString& NormalizedPackageName) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);

    const bool bResult = S && S->LastEnteredAt != FDateTime::MinValue();

    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::HasEnteredEver('%s'): state=%s LastEnteredAt=%s -> %s"),
        *NormalizedPackageName,
        S ? TEXT("yes") : TEXT("NO"),
        (S && S->LastEnteredAt != FDateTime::MinValue())
        ? *S->LastEnteredAt.ToIso8601()
        : TEXT("<none>"),
        bResult ? TEXT("true") : TEXT("false"));
    /*
    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::HasEnteredEver('%s'): state=%s LastEnteredAt=%s -> %s"),
        *NormalizedPackageName,
        S ? TEXT("yes") : TEXT("NO"),
        (S && S->LastEnteredAt != FDateTime::MinValue())
        ? *S->LastEnteredAt.ToIso8601()
        : TEXT("<none>"),
        bResult ? TEXT("true") : TEXT("false"));
    */
    return bResult;
}

// ─────────────────────────────────────────────────────────────────────────────
bool ULocationTrackerSubsystem::HasEnteredInWindow(const FString& NormalizedPackageName, float WindowMinutes) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);

    if (!S || S->LastEnteredAt == FDateTime::MinValue())
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasEnteredInWindow('%s', %g): no LastEnteredAt -> false"),
            *NormalizedPackageName, WindowMinutes);
        return false;
    }

    if (WindowMinutes <= 0.0f)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasEnteredInWindow('%s', %g): no window limit -> true"),
            *NormalizedPackageName, WindowMinutes);
        return true;
    }

    const FTimespan Window = FTimespan::FromMinutes(WindowMinutes);
    const FTimespan Elapsed = GetGameTimeNow() - S->LastEnteredAt;
    const bool bResult = (Elapsed <= Window);

    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::HasEnteredInWindow('%s', %g): Elapsed=%llds Window=%llds -> %s"),
        *NormalizedPackageName, WindowMinutes,
        (int64)Elapsed.GetTotalSeconds(), (int64)Window.GetTotalSeconds(),
        bResult ? TEXT("true") : TEXT("false"));

    return bResult;
}

// ─────────────────────────────────────────────────────────────────────────────
bool ULocationTrackerSubsystem::HasLeftAndReturned(const FString& NormalizedPackageName, float WindowMinutes) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);

    if (!S)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasLeftAndReturned('%s', %g): NO STATE -> false"),
            *NormalizedPackageName, WindowMinutes);
        return false;
    }

    if (S->EnterCount < 1)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasLeftAndReturned('%s', %g): EnterCount=%d < 1 -> false"),
            *NormalizedPackageName, WindowMinutes, S->EnterCount);
        return false;
    }

    if (S->LastLeftAt == FDateTime::MinValue() || S->LastEnteredAt == FDateTime::MinValue())
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasLeftAndReturned('%s', %g): LastEntered/LastLeft not set -> false"),
            *NormalizedPackageName, WindowMinutes);
        return false;
    }

    if (!(S->LastEnteredAt > S->LastLeftAt))
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasLeftAndReturned('%s', %g): LastEnteredAt=%s <= LastLeftAt=%s -> false"),
            *NormalizedPackageName, WindowMinutes,
            *S->LastEnteredAt.ToIso8601(), *S->LastLeftAt.ToIso8601());
        return false;
    }

    if (WindowMinutes > 0.0f)
    {
        const FTimespan Window = FTimespan::FromMinutes(WindowMinutes);
        const FTimespan Elapsed = GetGameTimeNow() - S->LastEnteredAt;
        const bool bInWindow = (Elapsed <= Window);
/*
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasLeftAndReturned('%s', %g): Enter=%d LastLeftAt=%s LastEnteredAt=%s Elapsed=%llds Window=%llds -> %s"),
            *NormalizedPackageName, WindowMinutes, S->EnterCount,
            *S->LastLeftAt.ToIso8601(), *S->LastEnteredAt.ToIso8601(),
            (int64)Elapsed.GetTotalSeconds(), (int64)Window.GetTotalSeconds(),
            bInWindow ? TEXT("true") : TEXT("false"));
*/
        UE_LOG(LogTemp, Verbose,
            TEXT("LocationTracker::HasLeftAndReturned('%s', %g): Enter=%d LastLeftAt=%s LastEnteredAt=%s Elapsed=%llds Window=%llds -> %s"),
            *NormalizedPackageName, WindowMinutes, S->EnterCount,
            *S->LastLeftAt.ToIso8601(), *S->LastEnteredAt.ToIso8601(),
            (int64)Elapsed.GetTotalSeconds(), (int64)Window.GetTotalSeconds(),
            bInWindow ? TEXT("true") : TEXT("false"));

        return bInWindow;
    }
/*
    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::HasLeftAndReturned('%s', %g): Enter=%d LastLeftAt=%s LastEnteredAt=%s (no window) -> true"),
        *NormalizedPackageName, WindowMinutes, S->EnterCount,
        *S->LastLeftAt.ToIso8601(), *S->LastEnteredAt.ToIso8601());
*/
    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::HasLeftAndReturned('%s', %g): Enter=%d (no window) -> true"),
        *NormalizedPackageName, WindowMinutes, S->EnterCount);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
int32 ULocationTrackerSubsystem::GetEnterCount(const FString& NormalizedPackageName) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);
    const int32 Result = S ? S->EnterCount : 0;

    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::GetEnterCount('%s'): state=%s -> %d"),
        *NormalizedPackageName,
        S ? TEXT("yes") : TEXT("NO"),
        Result);

    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
int32 ULocationTrackerSubsystem::GetLeaveCount(const FString& NormalizedPackageName) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);
    const int32 Result = S ? S->LeaveCount : 0;

    UE_LOG(LogTemp, Verbose,
        TEXT("LocationTracker::GetLeaveCount('%s'): state=%s -> %d"),
        *NormalizedPackageName,
        S ? TEXT("yes") : TEXT("NO"),
        Result);

    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> VisitsArray;
    for (const auto& Pair : VisitHistory)
    {
        const FLocationVisitState& S = Pair.Value;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("LevelPackageName"), S.LevelPackageName);
        Obj->SetStringField(TEXT("FirstEnteredAt"), S.FirstEnteredAt.ToIso8601());
        Obj->SetStringField(TEXT("LastEnteredAt"), S.LastEnteredAt.ToIso8601());
        Obj->SetStringField(TEXT("LastLeftAt"), S.LastLeftAt.ToIso8601());
        Obj->SetNumberField(TEXT("EnterCount"), S.EnterCount);
        Obj->SetNumberField(TEXT("LeaveCount"), S.LeaveCount);
        VisitsArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("Visits"), VisitsArray);

    // Сохраняем текущий уровень — при следующей загрузке сейва это даст
    // осмысленное «откуда пришли» для первого LevelLoaded в новой сессии.
    Root->SetStringField(TEXT("CachedCurrentLevelPackageName"), CachedCurrentLevelPackageName);

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

    // Восстанавливаем кэш текущего уровня из сейва — при первом LevelLoaded
    // после загрузки в логе будет осмысленное «откуда пришли».
    // Если поля в сейве нет (старый формат) — останется пустым,
    // и первый LevelLoaded покажет '<initial>'.
    FString CachedLevel;
    if (Root->TryGetStringField(TEXT("CachedCurrentLevelPackageName"), CachedLevel))
    {
        CachedCurrentLevelPackageName = CachedLevel;
    }
    else
    {
        CachedCurrentLevelPackageName.Empty();
    }

    const TArray<TSharedPtr<FJsonValue>>* VisitsArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Visits"), VisitsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *VisitsArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FLocationVisitState State;
            Obj->TryGetStringField(TEXT("LevelPackageName"), State.LevelPackageName);

            FString FirstStr, LastEnterStr, LastLeftStr;

            if (Obj->TryGetStringField(TEXT("FirstEnteredAt"), FirstStr))
            {
                if (!FDateTime::ParseIso8601(*FirstStr, State.FirstEnteredAt))
                    State.FirstEnteredAt = FDateTime::MinValue();
            }
            else
            {
                State.FirstEnteredAt = FDateTime::MinValue();
            }

            if (Obj->TryGetStringField(TEXT("LastEnteredAt"), LastEnterStr))
            {
                if (!FDateTime::ParseIso8601(*LastEnterStr, State.LastEnteredAt))
                    State.LastEnteredAt = FDateTime::MinValue();
            }
            else
            {
                State.LastEnteredAt = FDateTime::MinValue();
            }

            if (Obj->TryGetStringField(TEXT("LastLeftAt"), LastLeftStr))
            {
                if (!FDateTime::ParseIso8601(*LastLeftStr, State.LastLeftAt))
                    State.LastLeftAt = FDateTime::MinValue();
            }
            else
            {
                State.LastLeftAt = FDateTime::MinValue();
            }

            Obj->TryGetNumberField(TEXT("EnterCount"), State.EnterCount);
            Obj->TryGetNumberField(TEXT("LeaveCount"), State.LeaveCount);

            if (!State.LevelPackageName.IsEmpty())
            {
                VisitHistory.Add(State.LevelPackageName, State);
            }
        }
    }

    bSuppressNextLevelLoad = true;
    bLoadComplete = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// HandleVisitReset — обработчик EventBus-команды LocationVisitReset.
// Удаляет запись о посещении указанного уровня либо всю историю целиком.
// ─────────────────────────────────────────────────────────────────────────────
void ULocationTrackerSubsystem::HandleVisitReset(const FOutcomeEventBase& Outcome)
{
    ULocationVisitResetPayload* P = Cast<ULocationVisitResetPayload>(Outcome.Payload);
    if (!P) return;

    // ── Reset All ────────────────────────────────────────────────────────
    if (P->bResetAll)
    {
        const int32 Count = VisitHistory.Num();
        VisitHistory.Empty();

        UE_LOG(LogTemp, Log,
            TEXT("LocationTracker: Reset ALL visits (cleared %d entries)"), Count);
        return;
    }

    // ── Reset по иерархии локации ────────────────────────────────────────
    TArray<FString> Targets;
    if (!ResolveTargetPackageNames(
        P->TargetMap, P->TargetRegion, P->TargetStreet,
        P->TargetBuilding, P->TargetFloor, Targets)
        || Targets.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationTracker: Reset request with empty target (no assets resolved to scenes)"));
        return;
    }

    int32 TotalRemoved = 0;
    for (const FString& Norm : Targets)
    {
        TotalRemoved += VisitHistory.Remove(Norm);
    }

    UE_LOG(LogTemp, Log,
        TEXT("LocationTracker: Reset visits for %d target scene(s), removed=%d"),
        Targets.Num(), TotalRemoved);
}

// ─────────────────────────────────────────────────────────────────────────────
// ResolveTargetPackageNames — общий резолвер иерархии локации в список
// нормализованных имён пакетов. Используется в LocationVisitReset.
// ─────────────────────────────────────────────────────────────────────────────
bool ULocationTrackerSubsystem::ResolveTargetPackageNames(
    UWorldMapAsset* TargetMap,
    UWorldRegionAsset* TargetRegion,
    UStreetAsset* TargetStreet,
    UInteriorSetAsset* TargetBuilding,
    UFloorAsset* TargetFloor,
    TArray<FString>& OutPackageNames)
{
    OutPackageNames.Reset();

    // ── 1. Floor ─────────────────────────────────────────────────────────
    if (TargetFloor)
    {
        if (!TargetFloor->FloorLevel.IsNull())
        {
            const FString Norm = NormalizeLevelName(
                TargetFloor->FloorLevel.ToSoftObjectPath().GetLongPackageName());
            if (!Norm.IsEmpty())
                OutPackageNames.AddUnique(Norm);
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 2. Building — все этажи здания ───────────────────────────────────
    if (TargetBuilding)
    {
        for (const TSoftObjectPtr<UFloorAsset>& FloorRef : TargetBuilding->Floors)
        {
            UFloorAsset* Floor = FloorRef.LoadSynchronous();
            if (!Floor || Floor->FloorLevel.IsNull()) continue;

            const FString Norm = NormalizeLevelName(
                Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName());
            if (!Norm.IsEmpty())
                OutPackageNames.AddUnique(Norm);
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 3. Street — все этажи всех зданий улицы ──────────────────────────
    if (TargetStreet)
    {
        for (const TSoftObjectPtr<UInteriorSetAsset>& BuildingRef : TargetStreet->InteriorSets)
        {
            UInteriorSetAsset* Building = BuildingRef.LoadSynchronous();
            if (!Building) continue;

            for (const TSoftObjectPtr<UFloorAsset>& FloorRef : Building->Floors)
            {
                UFloorAsset* Floor = FloorRef.LoadSynchronous();
                if (!Floor || Floor->FloorLevel.IsNull()) continue;

                const FString Norm = NormalizeLevelName(
                    Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName());
                if (!Norm.IsEmpty())
                    OutPackageNames.AddUnique(Norm);
            }
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 4. Region — сцена региона ────────────────────────────────────────
    if (TargetRegion)
    {
        if (!TargetRegion->RegionLevel.IsNull())
        {
            const FString Norm = NormalizeLevelName(
                TargetRegion->RegionLevel.ToSoftObjectPath().GetLongPackageName());
            if (!Norm.IsEmpty())
                OutPackageNames.AddUnique(Norm);
        }
        return OutPackageNames.Num() > 0;
    }

    // ── 5. Map — сцены всех регионов карты ───────────────────────────────
    if (TargetMap)
    {
        for (const TSoftObjectPtr<UWorldRegionAsset>& RegionRef : TargetMap->Regions)
        {
            UWorldRegionAsset* Region = RegionRef.LoadSynchronous();
            if (!Region || Region->RegionLevel.IsNull()) continue;

            const FString Norm = NormalizeLevelName(
                Region->RegionLevel.ToSoftObjectPath().GetLongPackageName());
            if (!Norm.IsEmpty())
                OutPackageNames.AddUnique(Norm);
        }
        return OutPackageNames.Num() > 0;
    }

    return false;
}