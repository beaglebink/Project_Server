#include "LocationTrackerSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#include "LevelLoadedPayload.h"                        // сам payload рядом
#include "../SaveGame/GameSaveSubsystem.h"
#include "../ChoreSystem/ChoreManagerSubsystem.h"

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
    }

    bSuppressNextLevelLoad = true;

    UE_LOG(LogTemp, Warning, TEXT("LocationTracker[%p]: Initialize. Subscribed=%s, bSuppressNextLevelLoad=true"),
        this, LocationEventHandler.IsValid() ? TEXT("yes") : TEXT("NO"));
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
    }
    LocationEventCondition = nullptr;

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    VisitHistory.Empty();
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
    UE_LOG(LogTemp, Warning, TEXT("LocationTracker[%p]: HandleLocationEvent Type=%d Interior=%d"),
        this, (int32)Outcome.OutcomeType, (int32)Outcome.OutcomeInterior);

    if (Outcome.OutcomeType != EOutcomeType::Interior) return;

    if (Outcome.OutcomeInterior == EOutcomeInterior::LevelLoaded)
    {
        ULevelLoadedPayload* P = Cast<ULevelLoadedPayload>(Outcome.Payload);
        if (!P)
        {
            UE_LOG(LogTemp, Warning, TEXT("LocationTracker: LevelLoaded with no payload"));
            return;
        }

        const FString Norm = NormalizeLevelName(P->LevelPackageName);
        UE_LOG(LogTemp, Warning, TEXT("LocationTracker: LevelLoaded raw='%s' norm='%s' suppress=%s"),
            *P->LevelPackageName, *Norm, bSuppressNextLevelLoad ? TEXT("true") : TEXT("false"));

        if (Norm.IsEmpty()) return;

        FLocationVisitState& State = VisitHistory.FindOrAdd(Norm);
        State.LevelPackageName = Norm;

        const FDateTime Now = GetGameTimeNow();
        if (State.FirstEnteredAt == FDateTime::MinValue())
            State.FirstEnteredAt = Now;
        State.LastEnteredAt = Now;

        if (bSuppressNextLevelLoad)
        {
            bSuppressNextLevelLoad = false;
        }
        else
        {
            State.EnterCount++;
        }

        UE_LOG(LogTemp, Warning,
            TEXT("LocationTracker: LevelLoaded('%s') -> Enter=%d Leave=%d LastEnteredAt=%s LastLeftAt=%s"),
            *Norm, State.EnterCount, State.LeaveCount,
            *State.LastEnteredAt.ToIso8601(),
            State.LastLeftAt == FDateTime::MinValue() ? TEXT("<none>") : *State.LastLeftAt.ToIso8601());

        return;
    }

    if (Outcome.OutcomeInterior == EOutcomeInterior::FloorLeaving)
    {
        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Warning, TEXT("LocationTracker: FloorLeaving but no world"));
            return;
        }

        const FString Raw = World->GetOutermost()->GetName();
        const FString Norm = NormalizeLevelName(Raw);

        UE_LOG(LogTemp, Warning, TEXT("LocationTracker: FloorLeaving raw='%s' norm='%s'"),
            *Raw, *Norm);

        if (Norm.IsEmpty()) return;

        FLocationVisitState& State = VisitHistory.FindOrAdd(Norm);
        State.LevelPackageName = Norm;
        State.LastLeftAt = GetGameTimeNow();
        State.LeaveCount++;

        UE_LOG(LogTemp, Warning,
            TEXT("LocationTracker: FloorLeaving('%s') -> Enter=%d Leave=%d LastEnteredAt=%s LastLeftAt=%s"),
            *Norm, State.EnterCount, State.LeaveCount,
            State.LastEnteredAt == FDateTime::MinValue() ? TEXT("<none>") : *State.LastEnteredAt.ToIso8601(),
            *State.LastLeftAt.ToIso8601());
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

bool ULocationTrackerSubsystem::HasEnteredEver(const FString& NormalizedPackageName) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);
    return S && S->LastEnteredAt != FDateTime::MinValue();
}

bool ULocationTrackerSubsystem::HasEnteredInWindow(const FString& NormalizedPackageName, float WindowMinutes) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);
    if (!S || S->LastEnteredAt == FDateTime::MinValue()) return false;
    if (WindowMinutes <= 0.0f) return true;

    const FTimespan Window = FTimespan::FromMinutes(WindowMinutes);
    return (GetGameTimeNow() - S->LastEnteredAt) <= Window;
}

bool ULocationTrackerSubsystem::HasLeftAndReturned(const FString& NormalizedPackageName, float WindowMinutes) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);

    if (!S)
    {
        UE_LOG(LogTemp, Warning, TEXT("LocationTracker::HasLeftAndReturned('%s'): NO STATE"), *NormalizedPackageName);
        return false;
    }

    UE_LOG(LogTemp, Warning,
        TEXT("LocationTracker::HasLeftAndReturned('%s'): Enter=%d LastEnteredAt=%s LastLeftAt=%s Window=%g"),
        *NormalizedPackageName, S->EnterCount,
        S->LastEnteredAt == FDateTime::MinValue() ? TEXT("<none>") : *S->LastEnteredAt.ToIso8601(),
        S->LastLeftAt == FDateTime::MinValue() ? TEXT("<none>") : *S->LastLeftAt.ToIso8601(),
        WindowMinutes);

    if (S->EnterCount < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("  -> false: EnterCount < 1"));
        return false;
    }
    if (S->LastLeftAt == FDateTime::MinValue() || S->LastEnteredAt == FDateTime::MinValue())
    {
        UE_LOG(LogTemp, Warning, TEXT("  -> false: LastLeft/LastEntered not set"));
        return false;
    }
    if (!(S->LastEnteredAt > S->LastLeftAt))
    {
        UE_LOG(LogTemp, Warning, TEXT("  -> false: LastEnteredAt <= LastLeftAt"));
        return false;
    }

    if (WindowMinutes > 0.0f)
    {
        const FTimespan Window = FTimespan::FromMinutes(WindowMinutes);
        const FTimespan Elapsed = GetGameTimeNow() - S->LastEnteredAt;
        const bool bInWindow = (Elapsed <= Window);
        UE_LOG(LogTemp, Warning, TEXT("  -> window check: Elapsed=%llds Window=%llds -> %s"),
            (int64)Elapsed.GetTotalSeconds(), (int64)Window.GetTotalSeconds(),
            bInWindow ? TEXT("true") : TEXT("false"));
        return bInWindow;
    }

    UE_LOG(LogTemp, Warning, TEXT("  -> true"));
    return true;
}

int32 ULocationTrackerSubsystem::GetEnterCount(const FString& NormalizedPackageName) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);
    return S ? S->EnterCount : 0;
}

int32 ULocationTrackerSubsystem::GetLeaveCount(const FString& NormalizedPackageName) const
{
    const FLocationVisitState* S = FindVisitState(NormalizedPackageName);
    return S ? S->LeaveCount : 0;
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