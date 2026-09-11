#include "ChoreManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "TimerManager.h"
#include "JsonObjectConverter.h"
#include "ChoreHistoryConditionAsset.h"
#include "SaveGame/GameSaveSubsystem.h"
#include "ChorePayloads.h"
#include "MissionConditionAsset.h"
#include <MissionSubsystem.h>

void UChoreManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    TimerManager = &GetWorld()->GetTimerManager();

    // Загружаем все определения хор
    LoadAllDefinitions();

    // ---- Создание условий для подписок ----
    GlobalEventCondition = NewObject<UOutcomeConditionAsset>(this);
    GlobalEventCondition->OperatorType = EConditionOperator::Composite;
    GlobalEventCondition->FilterRow.OutcomeType = EOutcomeType::Default;
    GlobalEventCondition->CompileCondition();

    MissionRequestCondition = CreateSimpleMissionCondition(EOutcomeMission::ChoreStepRequest);

    ChoreCompletionCondition = NewObject<UOutcomeConditionAsset>(this);
    ChoreCompletionCondition->OperatorType = EConditionOperator::Composite;
    ChoreCompletionCondition->FilterRow.OutcomeType = EOutcomeType::Chore;
    ChoreCompletionCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    ChoreCompletionCondition->CompileCondition();

    // ---- Командные события ----
    AcceptRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::AcceptRequest);
    StartRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::StartRequest);
    CompleteRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::ChoreSucceeded);
    FailRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::ChoreFailed);
    ExpireRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::ExpireRequest);
    AbandonRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::AbandonRequest);
    RetryRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::RetryRequest);
    UnlockRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::UnlockRequest);
    RegisterChoreRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::RegisterChoreRequest);
    UnregisterChoreRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::UnregisterChoreRequest);
    ReacceptRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::ReacceptRequest);

    // ---- Регистрация обработчиков в EventBus ----
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        GlobalEventHandler = EventBus->RegisterHandler(GlobalEventCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleEvent));

        MissionRequestHandler = EventBus->RegisterHandler(MissionRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleMissionRequest));

        ChoreCompletionHandler = EventBus->RegisterHandler(ChoreCompletionCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleChoreCompletion));

        AcceptRequestHandler = EventBus->RegisterHandler(AcceptRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleAcceptRequest));
        StartRequestHandler = EventBus->RegisterHandler(StartRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleStartRequest));
        CompleteRequestHandler = EventBus->RegisterHandler(CompleteRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleCompleteRequest));
        FailRequestHandler = EventBus->RegisterHandler(FailRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleFailRequest));
        ExpireRequestHandler = EventBus->RegisterHandler(ExpireRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleExpireRequest));
        AbandonRequestHandler = EventBus->RegisterHandler(AbandonRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleAbandonRequest));
        RetryRequestHandler = EventBus->RegisterHandler(RetryRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleRetryRequest));
        UnlockRequestHandler = EventBus->RegisterHandler(UnlockRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleUnlockRequest));
        RegisterChoreRequestHandler = EventBus->RegisterHandler(RegisterChoreRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleRegisterChoreRequest));
        UnregisterChoreRequestHandler = EventBus->RegisterHandler(UnregisterChoreRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleUnregisterChoreRequest));
        ReacceptRequestHandler = EventBus->RegisterHandler(ReacceptRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleReacceptRequest));
    }

    // ---- Регистрация в системе сохранения ----
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    UE_LOG(LogTemp, Log, TEXT("ChoreManagerSubsystem: Initialized."));
}

void UChoreManagerSubsystem::Deinitialize()
{
    // Отписка от EventBus
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        auto Unreg = [&](FOutcomeHandlerHandle& Handle) {
            if (Handle.IsValid()) { EventBus->UnregisterHandler(Handle); Handle.Invalidate(); }
            };
        Unreg(GlobalEventHandler);
        Unreg(MissionRequestHandler);
        Unreg(ChoreCompletionHandler);
        Unreg(AcceptRequestHandler);
        Unreg(StartRequestHandler);
        Unreg(CompleteRequestHandler);
        Unreg(FailRequestHandler);
        Unreg(ExpireRequestHandler);
        Unreg(AbandonRequestHandler);
        Unreg(RetryRequestHandler);
        Unreg(UnlockRequestHandler);
        Unreg(ReacceptRequestHandler);
    }

    // Отписка обработчиков доступности
    for (auto& Pair : ActiveStates)
    {
        UnregisterReactivationHandler(Pair.Key);
        UnregisterAvailabilityHandler(Pair.Key);
    }
    // Очистка таймеров
    if (TimerManager)
    {
        for (auto& Pair : DeadlineTimers)
        {
            if (Pair.Value.IsValid()) TimerManager->ClearTimer(Pair.Value);
        }
        DeadlineTimers.Empty();
    }

    // Отписка от сохранения
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    ActiveStates.Empty();
    Definitions.Empty();
    History.Empty();

    Super::Deinitialize();
}

// ---- Загрузка определений ----
void UChoreManagerSubsystem::LoadAllDefinitions()
{
    UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
    if (!AssetManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChoreManager: AssetManager not initialized"));
        return;
    }

    TArray<FPrimaryAssetId> AssetIds;
    AssetManager->GetPrimaryAssetIdList(FPrimaryAssetType("Chore"), AssetIds);

    for (const FPrimaryAssetId& AssetId : AssetIds)
    {
        FSoftObjectPath Path = AssetManager->GetPrimaryAssetPath(AssetId);
        UChoreDefinition* Definition = Cast<UChoreDefinition>(Path.TryLoad());
        if (Definition)
        {
            RegisterChoreDefinition(Definition);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ChoreManager: Failed to load chore definition '%s'"), *AssetId.ToString());
        }
    }

    EvaluateAllAvailability();
}

void UChoreManagerSubsystem::RegisterChoreDefinition(UChoreDefinition* Definition)
{
    if (!Definition) return;
    FName ChoreId = Definition->GetChoreId();
    if (Definitions.Contains(ChoreId)) return;

    Definitions.Add(ChoreId, Definition);

    if (!ActiveStates.Contains(ChoreId))
    {
        FChoreState NewState;
        NewState.ChoreId = ChoreId;
        NewState.Status = EChoreStatus::Unavailable;
        ActiveStates.Add(ChoreId, NewState);
    }

    if (!Definition->AvailabilityCondition)
    {
        OfferChore(ChoreId);
    }
    else
    {
        RegisterAvailabilityHandler(Definition);
    }
}

void UChoreManagerSubsystem::RegisterAvailabilityHandler(UChoreDefinition* Definition)
{
    if (!Definition || !Definition->AvailabilityCondition)
        return;

    FName ChoreId = Definition->GetChoreId();

    // Если для этого задания уже зарегистрирован обработчик — выходим
    if (ActiveStates.Contains(ChoreId) && ActiveStates[ChoreId].AvailabilityHandler.IsValid())
        return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
        return;

    // Компилируем условие доступности
    Definition->AvailabilityCondition->CompileCondition();
    if (!Definition->AvailabilityCondition->GetCondition().IsValid())
        return;

    // Регистрируем обработчик на EventBus
    FOutcomeHandlerHandle Handle = EventBus->RegisterHandler(
        Definition->AvailabilityCondition,
        FOutcomeHandlerDelegate::CreateLambda([this, ChoreId](const FOutcomeEventBase&)
            {
                // При выполнении условия — предлагаем задание (реактивация)
                // Флаг bReactivated = true означает, что это повторное предложение
                // и будет вызван делегат OnChoreReactivated.
                UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
            })
    );

    if (Handle.IsValid())
    {
        ActiveStates[ChoreId].AvailabilityHandler = Handle;
    }
}

void UChoreManagerSubsystem::UnregisterAvailabilityHandler(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FOutcomeHandlerHandle& Handle = ActiveStates[ChoreId].AvailabilityHandler;
    if (!Handle.IsValid()) return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus) EventBus->UnregisterHandler(Handle);
    Handle.Invalidate();
}

// ---- Управление состоянием ----
void UChoreManagerSubsystem::UpdateChoreState(FName ChoreId, EChoreStatus NewStatus, bool bPublishEvent, bool bReactivated)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if(State.Status == NewStatus) return;

    State.Status = NewStatus;

    // Публикация события в EventBus (если требуется)
    if (bPublishEvent)
    {
        UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
        if (!EventBus) return;

        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;

        switch (NewStatus)
        {
        case EChoreStatus::Available:
        case EChoreStatus::Offered:
            Event.OutcomeChore = bReactivated ? EOutcomeChore::ReacceptRequest : EOutcomeChore::ChoreOffered;
            break;
        case EChoreStatus::Accepted:
            Event.OutcomeChore = EOutcomeChore::ChoreAccepted;
            break;
        case EChoreStatus::Active:
            Event.OutcomeChore = EOutcomeChore::ChoreStarted;
            break;
        case EChoreStatus::Succeeded:
            Event.OutcomeChore = EOutcomeChore::ChoreSucceeded;
            break;
        case EChoreStatus::Failed:
            Event.OutcomeChore = EOutcomeChore::ChoreFailed;
            break;
        case EChoreStatus::Expired:
            Event.OutcomeChore = EOutcomeChore::ChoreExpired;
            break;
        case EChoreStatus::RetryAvailable:
            Event.OutcomeChore = EOutcomeChore::ChoreRetryAvailable;
            break;
        case EChoreStatus::PendingReaccept:
            Event.OutcomeChore = EOutcomeChore::ChorePendingReaccept;
            break;
        default:
            return;
        }

        UChoreResultPayload* Payload = EventBus->CreatePayload<UChoreResultPayload>();
        if (Payload)
        {
            Payload->ChoreId = ChoreId;
            Payload->bSucceeded = (NewStatus == EChoreStatus::Succeeded);
            Payload->Performance = State.Performance;
            Event.Payload = Payload;
        }
        EventBus->PublishOutcome(Event);
    }
}

void UChoreManagerSubsystem::StartDeadlineTimer(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    const FChoreState& State = ActiveStates[ChoreId];
    if (State.Deadline == FDateTime::MinValue()) return;

    FTimespan Remaining = State.Deadline - FDateTime::UtcNow();
    if (Remaining.GetTotalSeconds() <= 0)
    {
        ExpireChore(ChoreId);
        return;
    }

    ClearDeadlineTimer(ChoreId);

    FTimerHandle Handle;
    TimerManager->SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UChoreManagerSubsystem::ExpireChore, ChoreId),
        (float)Remaining.GetTotalSeconds(), false);
    DeadlineTimers.Add(ChoreId, Handle);
}

void UChoreManagerSubsystem::ClearDeadlineTimer(FName ChoreId)
{
    if (DeadlineTimers.Contains(ChoreId))
    {
        FTimerHandle& Handle = DeadlineTimers[ChoreId];
        if (Handle.IsValid()) TimerManager->ClearTimer(Handle);
        DeadlineTimers.Remove(ChoreId);
    }
}

void UChoreManagerSubsystem::GrantRewards(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.bRewardIssued) return;

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def) return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChoreRewardPayload* Payload = EventBus->CreatePayload<UChoreRewardPayload>();
    if (Payload)
    {
        Payload->ChoreId = ChoreId;
        Payload->Rewards = Def->Rewards;
        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;
        Event.OutcomeChore = EOutcomeChore::ChoreRewardGranted;
        Event.Payload = Payload;
        EventBus->PublishOutcome(Event);
        State.bRewardIssued = true;
    }
}

void UChoreManagerSubsystem::AddHistoryEntry(FName ChoreId, EOutcomeChore Result, const FChorePerformanceMetrics& Performance)
{
    FChoreHistoryEntry Entry;
    Entry.ChoreId = ChoreId;
    Entry.Result = Result;
    Entry.Performance = Performance;
    Entry.Timestamp = FDateTime::UtcNow();
    History.Add(Entry);

    UE_LOG(LogTemp, Log, TEXT("AddHistoryEntry: History size now %d"), History.Num());

    if (History.Num() > 100)
    {
        History.RemoveAt(0, History.Num() - 100);
        UE_LOG(LogTemp, Log, TEXT("AddHistoryEntry: History trimmed to 100 entries"));
    }
}

// ---- Публичные методы управления (вызываются только из обработчиков) ----
void UChoreManagerSubsystem::OfferChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];

    // Задание уже принято или выполняется – не трогаем
    if (State.Status == EChoreStatus::Accepted || State.Status == EChoreStatus::Active)
        return;

    if (State.Status == EChoreStatus::Available || State.Status == EChoreStatus::Offered)
        return;

    if (State.Status == EChoreStatus::Succeeded && !Definitions[ChoreId]->bIsRepeatable)
        return;

    UpdateChoreState(ChoreId, EChoreStatus::Available);
}

void UChoreManagerSubsystem::AcceptChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Available && State.Status != EChoreStatus::Offered)
        return;

    State.AcceptTime = FDateTime::UtcNow();
    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def)
    {
        State.AttemptCount++;
    }

    EChoreStatus NewStatus = (Def && Def->bMustStartImmediately) ? EChoreStatus::Active : EChoreStatus::Accepted;
    UpdateChoreState(ChoreId, NewStatus);

    if (NewStatus == EChoreStatus::Active)
    {
        // ---- фиксируем фактический старт выполнения ----
        State.StartTime = FDateTime::UtcNow();

        if (Def && Def->Deadline != FTimespan::Zero())
        {
            State.Deadline = FDateTime::UtcNow() + Def->Deadline;
            StartDeadlineTimer(ChoreId);
        }
    }
}

void UChoreManagerSubsystem::StartChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Accepted && State.Status != EChoreStatus::WaitingToStart)
        return;

    // ---- фиксируем момент старта ----
    State.StartTime = FDateTime::UtcNow();

    // Пересчитываем дедлайн от текущего момента (начала выполнения)
    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def)
    {
        // Если дедлайн задан (не нулевой), пересчитываем от текущего времени
        if (Def->Deadline != FTimespan::Zero())
        {
            State.Deadline = FDateTime::UtcNow() + Def->Deadline;
        }
        else
        {
            State.Deadline = FDateTime::MinValue(); // без дедлайна
        }
    }

    UpdateChoreState(ChoreId, EChoreStatus::Active);
    StartDeadlineTimer(ChoreId);
}

void UChoreManagerSubsystem::CompleteChore(FName ChoreId, const FChorePerformanceMetrics& Performance)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);

    FChorePerformanceMetrics FinalPerformance = Performance;
    // Если время не пришло из payload — фиксируем фактическое.
    if (FinalPerformance.CompletionTimeSeconds <= 0.0f &&
        State.StartTime != FDateTime::MinValue())
    {
        FinalPerformance.CompletionTimeSeconds =
            (float)(FDateTime::UtcNow() - State.StartTime).GetTotalSeconds();
    }

    State.Performance = Performance;
    AddHistoryEntry(ChoreId, EOutcomeChore::CompleteRequest, Performance);
    UpdateChoreState(ChoreId, EChoreStatus::Succeeded);
    GrantRewards(ChoreId);

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def && Def->bIsRepeatable)
    {
        if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
        {
            UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
        {
            RegisterReactivationHandler(Def);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::RequireReaccept)
        {
            UpdateChoreState(ChoreId, EChoreStatus::PendingReaccept, true, false);
        }
    }
}

void UChoreManagerSubsystem::FailChore(FName ChoreId, const FChorePerformanceMetrics& Performance)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);
    EnsureElapsedTimeRecorded(State);
    UpdateChoreState(ChoreId, EChoreStatus::Failed);
    AddHistoryEntry(ChoreId, EOutcomeChore::FailRequest, State.Performance);

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def && Def->bIsRepeatable)
    {
        if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
        {
            UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
        {
            RegisterReactivationHandler(Def);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::RequireReaccept)
        {
            UpdateChoreState(ChoreId, EChoreStatus::PendingReaccept, true, false);
        }
    }
}

void UChoreManagerSubsystem::ExpireChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);
    EnsureElapsedTimeRecorded(State);
    UpdateChoreState(ChoreId, EChoreStatus::Expired);
    AddHistoryEntry(ChoreId, EOutcomeChore::ExpireRequest, State.Performance);

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def && Def->bIsRepeatable)
    {
        if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
        {
            UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
        {
            RegisterReactivationHandler(Def);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::RequireReaccept)
        {
            UpdateChoreState(ChoreId, EChoreStatus::PendingReaccept, true, false);
        }
    }
}

void UChoreManagerSubsystem::AbandonChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Accepted && State.Status != EChoreStatus::WaitingToStart && State.Status != EChoreStatus::Active)
        return;

    ClearDeadlineTimer(ChoreId);
    EnsureElapsedTimeRecorded(State);
    UpdateChoreState(ChoreId, EChoreStatus::Failed);
    AddHistoryEntry(ChoreId, EOutcomeChore::AbandonRequest, State.Performance);

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def && Def->bIsRepeatable)
    {
        if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
        {
            UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
        {
            RegisterReactivationHandler(Def);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::RequireReaccept)
        {
            UpdateChoreState(ChoreId, EChoreStatus::PendingReaccept, true, false);
        }
    }
}

void UChoreManagerSubsystem::RetryChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::PendingReaccept) return;

    State.bRewardIssued = false;
    State.Performance = FChorePerformanceMetrics();
    State.StartTime = FDateTime::MinValue();
    UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
}
void UChoreManagerSubsystem::UnlockChore(FName ChoreId)
{
    OfferChore(ChoreId);
}

void UChoreManagerSubsystem::RevokeChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];

    // Отзываем только если задание ещё не принято
    if (State.Status == EChoreStatus::Available || State.Status == EChoreStatus::Offered)
    {
        UnregisterAvailabilityHandler(ChoreId); // отписываемся от обработчика, если есть
        UpdateChoreState(ChoreId, EChoreStatus::Unavailable); // переводим в недоступное
        UE_LOG(LogTemp, Log, TEXT("ChoreManager: Revoked chore '%s' (condition no longer met)"), *ChoreId.ToString());
    }
}

// ---- Для миссий ----
void UChoreManagerSubsystem::RequestMissionChore(FName MissionId, FName ChoreId, int32 StepIndex)
{
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
    {
        UE_LOG(LogTemp, Error, TEXT("ChoreManager: RequestMissionChore - EventBus not available"));
        return;
    }

    UChoreMissionRequestPayload* Payload = EventBus->CreatePayload<UChoreMissionRequestPayload>();
    if (!Payload)
    {
        UE_LOG(LogTemp, Error, TEXT("ChoreManager: RequestMissionChore - failed to create payload"));
        return;
    }

    Payload->MissionId = MissionId;
    Payload->ChoreId = ChoreId;
    Payload->MissionStepIndex = StepIndex;

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Mission;
    Event.OutcomeMission = EOutcomeMission::ChoreStepRequest;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}

void UChoreManagerSubsystem::ReportMissionChoreResult(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance, FName MissionId)
{
    AddHistoryEntry(ChoreId, bSuccess ? EOutcomeChore::CompleteRequest : EOutcomeChore::FailRequest, Performance);

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChoreResultPayload* Payload = EventBus->CreatePayload<UChoreResultPayload>();
    if (Payload)
    {
        Payload->ChoreId = ChoreId;
        Payload->bSucceeded = bSuccess;
        Payload->Performance = Performance;
        Payload->MissionId = MissionId;

        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;
        Event.OutcomeChore = EOutcomeChore::ChoreMissionResult;
        Event.Payload = Payload;
        EventBus->PublishOutcome(Event);
    }
}

// ---- Запросы состояния (публичные) ----
EChoreStatus UChoreManagerSubsystem::GetChoreStatus(FName ChoreId) const
{
    if (ActiveStates.Contains(ChoreId)) return ActiveStates[ChoreId].Status;
    return EChoreStatus::Unavailable;
}

UChoreDefinition* UChoreManagerSubsystem::GetChoreDefinition(FName ChoreId) const
{
    if (Definitions.Contains(ChoreId)) return Definitions[ChoreId];
    return nullptr;
}

bool UChoreManagerSubsystem::IsChoreAvailable(FName ChoreId) const
{
    if (!ActiveStates.Contains(ChoreId)) return false;
    EChoreStatus Status = ActiveStates[ChoreId].Status;
    return Status == EChoreStatus::Available || Status == EChoreStatus::Offered;
}

TArray<FName> UChoreManagerSubsystem::GetAcceptedChoreIds() const
{
    TArray<FName> Result;
    for (const auto& Pair : ActiveStates)
    {
        EChoreStatus S = Pair.Value.Status;
        if (S == EChoreStatus::Accepted || S == EChoreStatus::WaitingToStart || S == EChoreStatus::Active)
            Result.Add(Pair.Key);
    }
    return Result;
}

TArray<FName> UChoreManagerSubsystem::GetActiveChoreIds() const
{
    TArray<FName> Result;
    for (const auto& Pair : ActiveStates)
    {
        if (Pair.Value.Status == EChoreStatus::Active)
            Result.Add(Pair.Key);
    }
    return Result;
}

UChoreDefinition* UChoreManagerSubsystem::GetChoreDefinitionByDisplayName(const FText& DisplayName) const
{
    for (const auto& Pair : Definitions)
    {
        const UChoreDefinition* Def = Pair.Value;
        if (Def && Def->DisplayName.EqualTo(DisplayName))
        {
            return const_cast<UChoreDefinition*>(Def);
        }
    }
    return nullptr;
}

TArray<FName> UChoreManagerSubsystem::GetAvailableChoreIds() const
{
    TArray<FName> Result;
    for (const auto& Pair : ActiveStates)
    {
        const EChoreStatus Status = Pair.Value.Status;
        if (Status == EChoreStatus::Available || Status == EChoreStatus::Offered)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

TArray<FName> UChoreManagerSubsystem::GetChoreIdsByDisplayName(const FText& DisplayName) const
{
    TArray<FName> Result;
    for (const auto& Pair : Definitions)
    {
        if (Pair.Value && Pair.Value->DisplayName.EqualTo(DisplayName))
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

// ---- Время выполнения ----
float UChoreManagerSubsystem::GetChoreElapsedTime(FName ChoreId) const
{
    if (!ActiveStates.Contains(ChoreId)) return 0.0f;
    const FChoreState& State = ActiveStates[ChoreId];

    // Если хора активна — считаем «живое» время от StartTime
    if (State.Status == EChoreStatus::Active && State.StartTime != FDateTime::MinValue())
    {
        return (float)(FDateTime::UtcNow() - State.StartTime).GetTotalSeconds();
    }

    // Иначе возвращаем сохранённое значение из метрик
    return State.Performance.CompletionTimeSeconds;
}

FDateTime UChoreManagerSubsystem::GetChoreStartTime(FName ChoreId) const
{
    if (ActiveStates.Contains(ChoreId)) return ActiveStates[ChoreId].StartTime;
    return FDateTime::MinValue();
}

void UChoreManagerSubsystem::SetChoreElapsedTime(FName ChoreId, float ElapsedSeconds)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];

    if (ElapsedSeconds < 0.0f) ElapsedSeconds = 0.0f;

    // Сдвигаем StartTime так, чтобы GetChoreElapsedTime() вернул нужное значение.
    State.StartTime = FDateTime::UtcNow() - FTimespan::FromSeconds(ElapsedSeconds);

    // Если хора уже завершена — пишем в метрики напрямую.
    if (State.Status != EChoreStatus::Active)
    {
        State.Performance.CompletionTimeSeconds = ElapsedSeconds;
    }
}

void UChoreManagerSubsystem::SetChoreStartTime(FName ChoreId, FDateTime InStartTime)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    ActiveStates[ChoreId].StartTime = InStartTime;
}

void UChoreManagerSubsystem::EnsureElapsedTimeRecorded(FChoreState& State) const
{
    // Если CompletionTimeSeconds не задано (<= 0) и есть StartTime — считаем сами.
    if (State.Performance.CompletionTimeSeconds <= 0.0f &&
        State.StartTime != FDateTime::MinValue())
    {
        State.Performance.CompletionTimeSeconds =
            (float)(FDateTime::UtcNow() - State.StartTime).GetTotalSeconds();
    }
}

float UChoreManagerSubsystem::ExtractMetricValue(const FChorePerformanceMetrics& Perf, EChorePerformanceMetric Metric)
{
    switch (Metric)
    {
    case EChorePerformanceMetric::CompletionTimeSeconds: return Perf.CompletionTimeSeconds;
    case EChorePerformanceMetric::Mistakes:              return Perf.Mistakes;
    case EChorePerformanceMetric::Accuracy:              return Perf.Accuracy;
    case EChorePerformanceMetric::Quantity:              return Perf.Quantity;
    default:                                             return 0.0f;
    }
}

bool UChoreManagerSubsystem::IsLowerBetterForMetric(EChorePerformanceMetric Metric)
{
    switch (Metric)
    {
    case EChorePerformanceMetric::CompletionTimeSeconds:
    case EChorePerformanceMetric::Mistakes:
        return true;
    case EChorePerformanceMetric::Accuracy:
    case EChorePerformanceMetric::Quantity:
        return false;
    default:
        return false;
    }
}

TArray<FName> UChoreManagerSubsystem::GetSucceededChoreIds() const
{
    TArray<FName> Result;
    for (const auto& Pair : ActiveStates)
    {
        if (Pair.Value.Status == EChoreStatus::Succeeded)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

// ---- Методы для условий истории ----
// ---- Расширенные запросы истории ----

bool UChoreManagerSubsystem::WasChoreEverCompleted(FName ChoreId) const
{
    if (ChoreId.IsNone()) return false;
    for (const FChoreHistoryEntry& Entry : History)
    {
        if (Entry.ChoreId == ChoreId && Entry.Result == EOutcomeChore::CompleteRequest)
            return true;
    }
    return false;
}

EOutcomeChore UChoreManagerSubsystem::GetLastOutcome(FName ChoreId) const
{
    if (ChoreId.IsNone()) return EOutcomeChore::Default;
    for (int32 i = History.Num() - 1; i >= 0; --i)
    {
        if (History[i].ChoreId == ChoreId)
            return History[i].Result;
    }
    return EOutcomeChore::Default;
}

FChorePerformanceMetrics UChoreManagerSubsystem::GetLastPerformance(FName ChoreId) const
{
    if (ChoreId.IsNone()) return FChorePerformanceMetrics();
    for (int32 i = History.Num() - 1; i >= 0; --i)
    {
        if (History[i].ChoreId == ChoreId)
            return History[i].Performance;
    }
    return FChorePerformanceMetrics();
}

int32 UChoreManagerSubsystem::GetHistoryCountByResult(
    FName ChoreId,
    EChoreFamily Family,
    EChoreSubtype Subtype,
    bool bUseFamily,
    bool bUseSubtype,
    EOutcomeChore RequiredResult,
    bool bRequireSpecificResult) const
{
    // Никаких фильтров — нечего считать.
    if (ChoreId.IsNone() && !bUseFamily && !bUseSubtype)
        return 0;

    int32 Count = 0;
    for (const FChoreHistoryEntry& Entry : History)
    {
        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;
        if (bRequireSpecificResult && Entry.Result != RequiredResult) continue;

        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family) continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }
        ++Count;
    }
    return Count;
}

int32 UChoreManagerSubsystem::GetSuccessCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
    bool bUseFamily, bool bUseSubtype) const
{
    return GetHistoryCountByResult(ChoreId, Family, Subtype, bUseFamily, bUseSubtype,
        EOutcomeChore::CompleteRequest, /*bRequireSpecificResult=*/true);
}

int32 UChoreManagerSubsystem::GetFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
    bool bUseFamily, bool bUseSubtype) const
{
    return GetHistoryCountByResult(ChoreId, Family, Subtype, bUseFamily, bUseSubtype,
        EOutcomeChore::FailRequest, /*bRequireSpecificResult=*/true);
}

int32 UChoreManagerSubsystem::GetExpireCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
    bool bUseFamily, bool bUseSubtype) const
{
    return GetHistoryCountByResult(ChoreId, Family, Subtype, bUseFamily, bUseSubtype,
        EOutcomeChore::ExpireRequest, /*bRequireSpecificResult=*/true);
}

int32 UChoreManagerSubsystem::GetAbandonCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
    bool bUseFamily, bool bUseSubtype) const
{
    return GetHistoryCountByResult(ChoreId, Family, Subtype, bUseFamily, bUseSubtype,
        EOutcomeChore::AbandonRequest, /*bRequireSpecificResult=*/true);
}

int32 UChoreManagerSubsystem::GetAnyFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
    bool bUseFamily, bool bUseSubtype) const
{
    return GetFailureCount(ChoreId, Family, Subtype, bUseFamily, bUseSubtype)
        + GetExpireCount(ChoreId, Family, Subtype, bUseFamily, bUseSubtype)
        + GetAbandonCount(ChoreId, Family, Subtype, bUseFamily, bUseSubtype);
}

int32 UChoreManagerSubsystem::GetTotalAttempts(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
    bool bUseFamily, bool bUseSubtype) const
{
    return GetHistoryCountByResult(ChoreId, Family, Subtype, bUseFamily, bUseSubtype,
        EOutcomeChore::Default, /*bRequireSpecificResult=*/false);
}

float UChoreManagerSubsystem::GetBestPerformanceFiltered(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, EChorePerformanceMetric Metric, bool bSucceededOnly) const
{
    const bool bLowerIsBetter = IsLowerBetterForMetric(Metric);

    float Best = 0.0f;
    bool bFirst = true;

    for (const FChoreHistoryEntry& Entry : History)
    {
        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;
        if (bSucceededOnly && Entry.Result != EOutcomeChore::CompleteRequest) continue;

        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family)  continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }

        const float Value = ExtractMetricValue(Entry.Performance, Metric);

        if (bFirst) { Best = Value; bFirst = false; }
        else if (bLowerIsBetter ? (Value < Best) : (Value > Best)) Best = Value;
    }

    return Best;
}

int32 UChoreManagerSubsystem::GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const
{
    // Если не задан фильтр, возвращаем 0
    if (ChoreId.IsNone() && !bUseFamily && !bUseSubtype)
    {
        UE_LOG(LogTemp, Warning, TEXT("GetHistoryCount: No filter specified, returning 0"));
        return 0;
    }

    int32 Count = 0;
    UE_LOG(LogTemp, Log, TEXT("GetHistoryCount: ChoreId='%s', bUseFamily=%d, Family=%d, bUseSubtype=%d, Subtype=%d, bSucceededOnly=%d"),
        *ChoreId.ToString(), bUseFamily, (int32)Family, bUseSubtype, (int32)Subtype, bSucceededOnly);

    for (const FChoreHistoryEntry& Entry : History)
    {
        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;
        if (bSucceededOnly && Entry.Result != EOutcomeChore::CompleteRequest) continue;

        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family) continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }
        Count++;
    }
    UE_LOG(LogTemp, Log, TEXT("GetHistoryCount: Result = %d"), Count);
    return Count;
}

float UChoreManagerSubsystem::GetBestPerformance(
    FName ChoreId,
    EChoreFamily Family,
    EChoreSubtype Subtype,
    bool bUseFamily,
    bool bUseSubtype,
    EChorePerformanceMetric Metric) const
{
    const bool bLowerIsBetter = IsLowerBetterForMetric(Metric);

    float Best = 0.0f;
    bool bFirst = true;

    for (const FChoreHistoryEntry& Entry : History)
    {
        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;

        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family)  continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }

        const float Value = ExtractMetricValue(Entry.Performance, Metric);

        if (bFirst) { Best = Value; bFirst = false; }
        else if (bLowerIsBetter ? (Value < Best) : (Value > Best)) Best = Value;
    }

    return Best;
}

bool UChoreManagerSubsystem::GetLastResult(FName ChoreId) const
{
    for (int32 i = History.Num() - 1; i >= 0; --i)
    {
        if (History[i].ChoreId == ChoreId)
            return History[i].Result == EOutcomeChore::CompleteRequest;
    }
    return false;
}

// ---- Обработчики событий ----
void UChoreManagerSubsystem::HandleEvent(const FOutcomeEventBase& Outcome)
{
    UE_LOG(LogTemp, Log, TEXT("HandleEvent: Type=%d, Mission=%d, Chore=%d"),
        (int32)Outcome.OutcomeType, (int32)Outcome.OutcomeMission, (int32)Outcome.OutcomeChore);

    // Игнорируем командные события (заканчиваются на "Request")
    // Они не должны влиять на доступность заданий
    if (Outcome.OutcomeType == EOutcomeType::Chore)
    {
        EOutcomeChore Chore = Outcome.OutcomeChore;
        if (Chore == EOutcomeChore::AcceptRequest ||
            Chore == EOutcomeChore::StartRequest ||
            Chore == EOutcomeChore::CompleteRequest ||
            Chore == EOutcomeChore::FailRequest ||
            Chore == EOutcomeChore::ExpireRequest ||
            Chore == EOutcomeChore::AbandonRequest ||
            Chore == EOutcomeChore::RetryRequest ||
            Chore == EOutcomeChore::UnlockRequest)
        {
            return; // Командные события не влияют на доступность
        }
    }

    // Проходим по всем заданиям
// Проходим по всем заданиям
    for (auto& Pair : ActiveStates)
    {
        FName ChoreId = Pair.Key;
        FChoreState& State = Pair.Value;

        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (!Def || !Def->AvailabilityCondition) continue;

        if (!Def->AvailabilityCondition->GetCondition().IsValid())
        {
            Def->AvailabilityCondition->CompileCondition();
        }
        if (!Def->AvailabilityCondition->GetCondition().IsValid()) continue;

        bool bConditionMet = Def->AvailabilityCondition->GetCondition()->Evaluate(Outcome);

        // Логируем результат проверки (можно оставить Verbose)
        UE_LOG(LogTemp, Verbose, TEXT("HandleEvent: Chore '%s' condition met = %s, status = %d"),
            *ChoreId.ToString(), bConditionMet ? TEXT("true") : TEXT("false"), (int32)State.Status);

        if (State.Status == EChoreStatus::Unavailable || State.Status == EChoreStatus::RetryAvailable)
        {
            if (bConditionMet)
            {
                OfferChore(ChoreId);
            }
        }
        else if (State.Status == EChoreStatus::Available || State.Status == EChoreStatus::Offered)
        {
            bool bShouldRevoke = true;

            // Проверяем, является ли условие событийным (не должно отзываться)
            if (UMissionConditionAsset* MissionCond = Cast<UMissionConditionAsset>(Def->AvailabilityCondition))
            {
                // Для IsCompleted, IsFailed, IsAbandoned отключаем отзыв полностью
                if (MissionCond->ConditionType == EMissionConditionType::IsCompleted ||
                    MissionCond->ConditionType == EMissionConditionType::IsFailed ||
                    MissionCond->ConditionType == EMissionConditionType::IsAbandoned)
                {
                    bShouldRevoke = false;
                }
                // Для StepReached отключаем отзыв только для событий, не связанных с миссией
                else if (MissionCond->ConditionType == EMissionConditionType::StepReached)
                {
                    if (Outcome.OutcomeType != EOutcomeType::Mission)
                        bShouldRevoke = false;
                }
            }

            if (UMissionConditionAsset* MissionCond = Cast<UMissionConditionAsset>(Def->AvailabilityCondition))
            {
                if (MissionCond->ConditionType == EMissionConditionType::IsCompleted || 
                    MissionCond->ConditionType == EMissionConditionType::IsFailed || 
                    MissionCond->ConditionType == EMissionConditionType::IsAbandoned)
                {
                    UMissionAsset* MissionAsset = MissionCond->MissionAsset;
                    FName MissionName = MissionAsset ? MissionAsset->GetMissionId() : FName();
                    UMissionSubsystem* MissionSys = GetGameInstance()->GetSubsystem<UMissionSubsystem>();
                    if (MissionSys && MissionSys->IsMissionActive(MissionName))
                    {
                        bShouldRevoke = true;
                    }
                }
            }

            if (!bConditionMet && (bShouldRevoke))
            {
                RevokeChore(ChoreId);
            }
        }
    }
}

void UChoreManagerSubsystem::HandleMissionRequest(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeMission != EOutcomeMission::ChoreStepRequest)
        return;

    UChoreMissionRequestPayload* Req = Cast<UChoreMissionRequestPayload>(Outcome.Payload);
    if (!Req) return;

    FName ChoreId = Req->ChoreId;
    if (!Definitions.Contains(ChoreId))
    {
        UE_LOG(LogTemp, Warning, TEXT("ChoreManager: Mission request for unknown ChoreId '%s'"), *ChoreId.ToString());
        return;
    }

    if (!ActiveStates.Contains(ChoreId))
    {
        FChoreState NewState;
        NewState.ChoreId = ChoreId;
        NewState.Status = EChoreStatus::Active;
        NewState.StartTime = FDateTime::UtcNow();
        ActiveStates.Add(ChoreId, NewState);
    }
    else
    {
        FChoreState& State = ActiveStates[ChoreId];
        if (State.Status == EChoreStatus::Active || State.Status == EChoreStatus::Succeeded)
            return;
        State.Status = EChoreStatus::Active;
        State.StartTime = FDateTime::UtcNow();
    }
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;
        Event.OutcomeChore = EOutcomeChore::ChoreStarted;
        Event.Payload = Req;
        EventBus->PublishOutcome(Event);
    }
}

void UChoreManagerSubsystem::HandleChoreCompletion(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeChore != EOutcomeChore::ChoreSucceeded &&
        Outcome.OutcomeChore != EOutcomeChore::ChoreFailed &&
        Outcome.OutcomeChore != EOutcomeChore::ChoreExpired &&
        Outcome.OutcomeChore != EOutcomeChore::ChoreRetryAvailable)
    {
        return;
    }

    UChoreResultPayload* Result = Cast<UChoreResultPayload>(Outcome.Payload);
    if (!Result) return;

    FName ChoreId = Result->ChoreId;
    if (!ActiveStates.Contains(ChoreId)) return;

    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);
    State.Performance = Result->Performance;
    State.bSucceeded = Result->bSucceeded;

    if (Result->bSucceeded)
    {
        UpdateChoreState(ChoreId, EChoreStatus::Succeeded);
        GrantRewards(ChoreId);
        AddHistoryEntry(ChoreId, EOutcomeChore::CompleteRequest, Result->Performance);

        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def && Def->bIsRepeatable)
        {
            if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
            {
                UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
            }
            else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
            {
                RegisterReactivationHandler(Def);
            }
            else if (Def->RetryBehavior == EChoreRetryBehavior::RequireReaccept)
            {
                UpdateChoreState(ChoreId, EChoreStatus::PendingReaccept, true, false);
            }
        }
    }
    else
    {
        UpdateChoreState(ChoreId, EChoreStatus::Failed);
        AddHistoryEntry(ChoreId, EOutcomeChore::FailRequest, Result->Performance);

        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def && Def->bIsRepeatable)
        {
            if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
            {
                UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
            }
            else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
            {
                RegisterReactivationHandler(Def);
            }
            else if (Def->RetryBehavior == EChoreRetryBehavior::RequireReaccept)
            {
                UpdateChoreState(ChoreId, EChoreStatus::PendingReaccept, true, false);
            }
        }
    }

    if (!Result->MissionId.IsNone())
    {
        UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
        if (EventBus)
        {
            FOutcomeEventBase Event;
            Event.OutcomeType = EOutcomeType::Chore;
            Event.OutcomeChore = EOutcomeChore::ChoreMissionResult;
            Event.Payload = Result;
            EventBus->PublishOutcome(Event);
        }
    }
}

// ---- Реализация обработчиков команд ----
void UChoreManagerSubsystem::HandleAcceptRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;

    UE_LOG(LogTemp, Warning, TEXT("ChoreManager: HandleAcceptRequest %s"), *Payload->ChoreId.ToString());

    AcceptChore(Payload->ChoreId);
}

void UChoreManagerSubsystem::HandleStartRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    StartChore(Payload->ChoreId);
}

void UChoreManagerSubsystem::HandleCompleteRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    CompleteChore(Payload->ChoreId, Payload->Performance);
}

void UChoreManagerSubsystem::HandleFailRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    FailChore(Payload->ChoreId, Payload->Performance);
}

void UChoreManagerSubsystem::HandleExpireRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    ExpireChore(Payload->ChoreId);
}

void UChoreManagerSubsystem::HandleAbandonRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    AbandonChore(Payload->ChoreId);
}

void UChoreManagerSubsystem::HandleRetryRequest(const FOutcomeEventBase& Outcome)
{
    UChoreReacceptPayload* Payload = Cast<UChoreReacceptPayload>(Outcome.Payload);
    if (!Payload) return;
    RetryChore(Payload->ChoreId);
}

void UChoreManagerSubsystem::HandleUnlockRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    UnlockChore(Payload->ChoreId);
}

// ---- Вспомогательные функции ----
UOutcomeConditionAsset* UChoreManagerSubsystem::CreateSimpleChoreCondition(EOutcomeChore ChoreType)
{
    UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>(this);
    Asset->OperatorType = EConditionOperator::Composite;
    Asset->FilterRow.OutcomeType = EOutcomeType::Chore;
    Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    Asset->FilterRow.ChoreType = ChoreType;
    Asset->FilterRow.ChoreComparison = EConditionComparison::Equals;
    Asset->CompileCondition();
    return Asset;
}

UOutcomeConditionAsset* UChoreManagerSubsystem::CreateSimpleMissionCondition(EOutcomeMission MissionType)
{
    UOutcomeConditionAsset* Asset = NewObject<UOutcomeConditionAsset>(this);
    Asset->OperatorType = EConditionOperator::Composite;
    Asset->FilterRow.OutcomeType = EOutcomeType::Mission;
    Asset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    Asset->FilterRow.MissionType = MissionType;
    Asset->FilterRow.MissionComparison = EConditionComparison::Equals;
    Asset->CompileCondition();
    return Asset;
}

void UChoreManagerSubsystem::EvaluateAllAvailability()
{
    for (auto& Pair : ActiveStates)
    {
        FName ChoreId = Pair.Key;
        if (Pair.Value.Status == EChoreStatus::Unavailable)
        {
            UChoreDefinition* Def = GetChoreDefinition(ChoreId);
            if (Def && Def->AvailabilityCondition)
            {
                if (!Def->AvailabilityCondition->GetCondition().IsValid())
                    Def->AvailabilityCondition->CompileCondition();
                if (Def->AvailabilityCondition->GetCondition().IsValid())
                {
                    FOutcomeEventBase Dummy;
                    if (Def->AvailabilityCondition->GetCondition()->Evaluate(Dummy))
                    {
                        OfferChore(ChoreId);
                    }
                }
            }
        }
    }
}

void UChoreManagerSubsystem::RegisterReactivationHandler(UChoreDefinition* Definition)
{
    if (!Definition || !Definition->ReactivationCondition)
        return;

    FName ChoreId = Definition->GetChoreId();

    // Если уже есть активный обработчик – не создаём новый
    if (ActiveStates.Contains(ChoreId) && ActiveStates[ChoreId].AvailabilityHandler.IsValid())
        return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
        return;

    // Компилируем условие
    Definition->ReactivationCondition->CompileCondition();
    if (!Definition->ReactivationCondition->GetCondition().IsValid())
        return;

    // Проверяем, выполнено ли условие прямо сейчас
    FOutcomeEventBase Dummy;
    Dummy.OutcomeType = EOutcomeType::Default;
    if (Definition->ReactivationCondition->GetCondition()->Evaluate(Dummy))
    {
        // Условие уже истинно – реактивируем сразу
        UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        return;
    }

    // Иначе подписываемся на событие
    FOutcomeHandlerHandle Handle = EventBus->RegisterHandler(
        Definition->ReactivationCondition,
        FOutcomeHandlerDelegate::CreateLambda([this, ChoreId](const FOutcomeEventBase&)
            {
                UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
            })
    );

    if (Handle.IsValid())
    {
        ActiveStates[ChoreId].ReactivationHandler = Handle; // <-- ИЗМЕНЕНО
    }
}

void UChoreManagerSubsystem::UnregisterReactivationHandler(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FOutcomeHandlerHandle& Handle = ActiveStates[ChoreId].ReactivationHandler;
    if (!Handle.IsValid()) return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus) EventBus->UnregisterHandler(Handle);
    Handle.Invalidate();
}

// ---- ISaveableSubsystem ----
void UChoreManagerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // Сохраняем состояния
    TArray<TSharedPtr<FJsonValue>> StateArray;
    for (const auto& Pair : ActiveStates)
    {
        const FChoreState& State = Pair.Value;
        if (State.Status == EChoreStatus::Unavailable) continue;

        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("ChoreId"), State.ChoreId.ToString());
        Obj->SetNumberField(TEXT("Status"), (uint8)State.Status);
        Obj->SetStringField(TEXT("AcceptTime"), State.AcceptTime.ToIso8601());
        Obj->SetStringField(TEXT("StartTime"), State.StartTime.ToIso8601());
        Obj->SetStringField(TEXT("Deadline"), State.Deadline.ToIso8601());
        Obj->SetNumberField(TEXT("AttemptCount"), State.AttemptCount);
        Obj->SetBoolField(TEXT("bSucceeded"), State.bSucceeded);
        Obj->SetBoolField(TEXT("bRewardIssued"), State.bRewardIssued);

        TSharedPtr<FJsonObject> PerfObj = MakeShared<FJsonObject>();
        PerfObj->SetNumberField(TEXT("CompletionTimeSeconds"), State.Performance.CompletionTimeSeconds);
        PerfObj->SetNumberField(TEXT("Mistakes"), State.Performance.Mistakes);
        PerfObj->SetNumberField(TEXT("Accuracy"), State.Performance.Accuracy);
        PerfObj->SetNumberField(TEXT("Quantity"), State.Performance.Quantity);
        Obj->SetObjectField(TEXT("Performance"), PerfObj);
        StateArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("States"), StateArray);

    // Сохраняем историю
    TArray<TSharedPtr<FJsonValue>> HistoryArray;
    for (const FChoreHistoryEntry& Entry : History)
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("ChoreId"), Entry.ChoreId.ToString());
        Obj->SetNumberField(TEXT("Result"), (uint8)Entry.Result);
        Obj->SetStringField(TEXT("Timestamp"), Entry.Timestamp.ToIso8601());

        TSharedPtr<FJsonObject> PerfObj = MakeShared<FJsonObject>();
        PerfObj->SetNumberField(TEXT("CompletionTimeSeconds"), Entry.Performance.CompletionTimeSeconds);
        PerfObj->SetNumberField(TEXT("Mistakes"), Entry.Performance.Mistakes);
        PerfObj->SetNumberField(TEXT("Accuracy"), Entry.Performance.Accuracy);
        PerfObj->SetNumberField(TEXT("Quantity"), Entry.Performance.Quantity);
        Obj->SetObjectField(TEXT("Performance"), PerfObj);
        HistoryArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("History"), HistoryArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void UChoreManagerSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bLoadComplete = false;
    FString SerializedDataCopy = InData.SerializedData;
    if (SerializedDataCopy.IsEmpty())
    {
        bLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SerializedDataCopy);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        bLoadComplete = true;
        return;
    }

    ActiveStates.Empty();
    History.Empty();
    DeadlineTimers.Empty();

    // Восстанавливаем состояния
    const TArray<TSharedPtr<FJsonValue>>* StateArray = nullptr;
    if (Root->TryGetArrayField(TEXT("States"), StateArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *StateArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FChoreState State;
            FString ChoreIdStr, AcceptTimeStr, DeadlineStr;
            Obj->TryGetStringField(TEXT("ChoreId"), ChoreIdStr);
            State.ChoreId = FName(*ChoreIdStr);
            int32 StatusInt = 0;
            Obj->TryGetNumberField(TEXT("Status"), StatusInt);
            State.Status = (EChoreStatus)StatusInt;
            Obj->TryGetStringField(TEXT("AcceptTime"), AcceptTimeStr);
            FDateTime::ParseIso8601(*AcceptTimeStr, State.AcceptTime);

            FString StartTimeStr;
            if (Obj->TryGetStringField(TEXT("StartTime"), StartTimeStr))
            {
                if (!FDateTime::ParseIso8601(*StartTimeStr, State.StartTime))
                {
                    State.StartTime = FDateTime::MinValue();
                }
            }
            else
            {
                State.StartTime = FDateTime::MinValue();
            }

            Obj->TryGetStringField(TEXT("Deadline"), DeadlineStr);
            FDateTime::ParseIso8601(*DeadlineStr, State.Deadline);            Obj->TryGetNumberField(TEXT("AttemptCount"), State.AttemptCount);
            Obj->TryGetBoolField(TEXT("bSucceeded"), State.bSucceeded);
            Obj->TryGetBoolField(TEXT("bRewardIssued"), State.bRewardIssued);

            const TSharedPtr<FJsonObject>* PerfObj = nullptr;
            if (Obj->TryGetObjectField(TEXT("Performance"), PerfObj))
            {
                (*PerfObj)->TryGetNumberField(TEXT("CompletionTimeSeconds"), State.Performance.CompletionTimeSeconds);
                (*PerfObj)->TryGetNumberField(TEXT("Mistakes"), State.Performance.Mistakes);
                (*PerfObj)->TryGetNumberField(TEXT("Accuracy"), State.Performance.Accuracy);
                (*PerfObj)->TryGetNumberField(TEXT("Quantity"), State.Performance.Quantity);
            }

            ActiveStates.Add(State.ChoreId, State);

            if (State.Status == EChoreStatus::Active && State.Deadline != FDateTime::MinValue())
            {
                FTimespan Remaining = State.Deadline - FDateTime::UtcNow();
                if (Remaining.GetTotalSeconds() > 0)
                {
                    FTimerHandle Handle;
                    TimerManager->SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UChoreManagerSubsystem::ExpireChore, State.ChoreId),
                        (float)Remaining.GetTotalSeconds(), false);
                    DeadlineTimers.Add(State.ChoreId, Handle);
                }
                else
                {
                    ExpireChore(State.ChoreId);
                }
            }
        }
    }

    // Восстанавливаем историю
    const TArray<TSharedPtr<FJsonValue>>* HistoryArray = nullptr;
    if (Root->TryGetArrayField(TEXT("History"), HistoryArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *HistoryArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FChoreHistoryEntry Entry;
            FString ChoreIdStr, TimestampStr;
            Obj->TryGetStringField(TEXT("ChoreId"), ChoreIdStr);
            Entry.ChoreId = FName(*ChoreIdStr);
            int32 ResultInt = 0;
            Obj->TryGetStringField(TEXT("Timestamp"), TimestampStr);
            FDateTime::ParseIso8601(*TimestampStr, Entry.Timestamp);

            const TSharedPtr<FJsonObject>* PerfObj = nullptr;
            if (Obj->TryGetObjectField(TEXT("Performance"), PerfObj))
            {
                (*PerfObj)->TryGetNumberField(TEXT("CompletionTimeSeconds"), Entry.Performance.CompletionTimeSeconds);
                (*PerfObj)->TryGetNumberField(TEXT("Mistakes"), Entry.Performance.Mistakes);
                (*PerfObj)->TryGetNumberField(TEXT("Accuracy"), Entry.Performance.Accuracy);
                (*PerfObj)->TryGetNumberField(TEXT("Quantity"), Entry.Performance.Quantity);
            }

            History.Add(Entry);
        }
    }

    // После загрузки перепроверяем доступность
    EvaluateAllAvailability();
    bLoadComplete = true;
}

void UChoreManagerSubsystem::HandleRegisterChoreRequest(const FOutcomeEventBase& Outcome)
{
    UChoreRegisterPayload* Payload = Cast<UChoreRegisterPayload>(Outcome.Payload);
    if (!Payload || !Payload->Definition)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleRegisterChoreRequest: invalid payload or Definition is null"));
        return;
    }

    RegisterChoreDefinition(Payload->Definition);
    UE_LOG(LogTemp, Log, TEXT("HandleRegisterChoreRequest: registered chore '%s'"), *Payload->Definition->GetChoreId().ToString());
}

void UChoreManagerSubsystem::HandleUnregisterChoreRequest(const FOutcomeEventBase& Outcome)
{
    UChoreUnregisterPayload* Payload = Cast<UChoreUnregisterPayload>(Outcome.Payload);
    if (!Payload)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreRequest: invalid payload"));
        return;
    }

    FName ChoreId = Payload->ChoreId;
    if (ChoreId.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreRequest: ChoreId is None"));
        return;
    }

    // Проверяем, существует ли задание
    if (!Definitions.Contains(ChoreId))
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreRequest: chore '%s' not found"), *ChoreId.ToString());
        return;
    }

    // Проверяем статус
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (State)
    {
        EChoreStatus Status = State->Status;
        // Разрешаем удаление, если задание завершено, истекло, недоступно или принудительно
        bool bCanRemove = (Status == EChoreStatus::Unavailable) ||
            (Status == EChoreStatus::Expired) ||
            (Status == EChoreStatus::Succeeded) ||
            (Status == EChoreStatus::Failed) ||
            Payload->bForceRemove;

        if (!bCanRemove)
        {
            UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreRequest: chore '%s' is in status %d and cannot be removed (use bForceRemove=true to override)"),
                *ChoreId.ToString(), (int32)Status);
            return;
        }

        // Если принудительно удаляем активное задание – завершаем его
        if (Payload->bForceRemove && (Status == EChoreStatus::Accepted || Status == EChoreStatus::Active || Status == EChoreStatus::WaitingToStart))
        {
            // Можно вызвать AbandonChore или просто перевести в Failed
            // Здесь для простоты переведём в Failed и запишем историю
            if (Status == EChoreStatus::Active)
            {
                ClearDeadlineTimer(ChoreId);
            }
            State->bSucceeded = false;
            AddHistoryEntry(ChoreId, EOutcomeChore::FailRequest, State->Performance);
            UpdateChoreState(ChoreId, EChoreStatus::Failed);
        }
    }

    // Удаляем из Definitions и ActiveStates
    Definitions.Remove(ChoreId);
    if (State)
    {
        ActiveStates.Remove(ChoreId);
    }

    // Отписываем обработчик доступности, если есть
    UnregisterAvailabilityHandler(ChoreId);

    // Останавливаем таймер, если он ещё висит (на всякий случай)
    ClearDeadlineTimer(ChoreId);

    UE_LOG(LogTemp, Log, TEXT("HandleUnregisterChoreRequest: chore '%s' unregistered"), *ChoreId.ToString());
}

void UChoreManagerSubsystem::HandleReacceptRequest(const FOutcomeEventBase& Outcome)
{
    UChoreReacceptPayload* Payload = Cast<UChoreReacceptPayload>(Outcome.Payload);
    if (!Payload) return;

    FName ChoreId = Payload->ChoreId;
    if (!ActiveStates.Contains(ChoreId)) return;

    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::PendingReaccept) return;

    // Реактивируем задание
    UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
}
