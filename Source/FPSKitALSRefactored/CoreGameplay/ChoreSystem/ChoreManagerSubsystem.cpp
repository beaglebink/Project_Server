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
#include <LocationTrackerSubsystem/LocationTrackerSubsystem.h>

void UChoreManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Форсируем инициализацию системы сохранения ДО нас.
    // Без этого GetSubsystem<UGameSaveSubsystem>() может вернуть nullptr,
    // и регистрация Saveable-подсистемы молча не сработает.
    Collection.InitializeDependency<UGameSaveSubsystem>();
    Collection.InitializeDependency<ULocationTrackerSubsystem>();

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

    AdvanceStageRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::AdvanceStageRequest);
    PauseRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::PauseRequest);
    ResumeRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::ResumeRequest);

    // ---- Deferred evaluation & Actor Lifecycle ----
    RegisterChoreActorCondition = CreateSimpleChoreCondition(EOutcomeChore::RegisterChoreActor);
    UnregisterChoreActorCondition = CreateSimpleChoreCondition(EOutcomeChore::UnregisterChoreActor);
    DeferredEvaluationRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::DeferredEvaluationRequest);
    ReplayDeferredResultRequestCondition = CreateSimpleChoreCondition(EOutcomeChore::ReplayDeferredResultRequest);
    DeferredResultReportCondition = CreateSimpleChoreCondition(EOutcomeChore::DeferredResultReport);

    SaveChoreActorDataCondition = CreateSimpleChoreCondition(EOutcomeChore::SaveChoreActorData);

    // ---- Регистрация обработчиков в EventBus ----
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        GlobalEventHandler = EventBus->RegisterHandler(GlobalEventCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleEvent));

        //MissionRequestHandler = EventBus->RegisterHandler(MissionRequestCondition,
        //    FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleMissionRequest));

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
        AdvanceStageRequestHandler = EventBus->RegisterHandler(AdvanceStageRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleAdvanceStageRequest));
        PauseRequestHandler = EventBus->RegisterHandler(PauseRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandlePauseRequest));
        ResumeRequestHandler = EventBus->RegisterHandler(ResumeRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleResumeRequest));

        // ---- Deferred evaluation & Actor Lifecycle ----
        RegisterChoreActorHandler = EventBus->RegisterHandler(RegisterChoreActorCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleRegisterChoreActor));

        UnregisterChoreActorHandler = EventBus->RegisterHandler(UnregisterChoreActorCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleUnregisterChoreActor));

        DeferredEvaluationRequestHandler = EventBus->RegisterHandler(DeferredEvaluationRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleDeferredEvaluationRequest));

        ReplayDeferredResultRequestHandler = EventBus->RegisterHandler(ReplayDeferredResultRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleReplayDeferredResultRequest));

        DeferredResultReportHandler = EventBus->RegisterHandler(DeferredResultReportCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleDeferredResultReport));

        SaveChoreActorDataHandler = EventBus->RegisterHandler(SaveChoreActorDataCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleSaveChoreActorData));
    }

    // ---- Регистрация в системе сохранения ----
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    // ---- Availability pulse ----
    // Раз в N секунд напрямую перепроверяем state-driven availability
    // conditions, чтобы time-based условия срабатывали сами по себе.
    // EventBus не задействован — pulse нужен только этой подсистеме.
    if (TimerManager)
    {
        TimerManager->SetTimer(
            AvailabilityPulseHandle,
            FTimerDelegate::CreateUObject(this, &UChoreManagerSubsystem::EvaluateAllAvailability),
            AvailabilityPulseIntervalSeconds,
            /*bLoop=*/true);
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
        Unreg(RegisterChoreRequestHandler);
        Unreg(UnregisterChoreRequestHandler);
        Unreg(ReacceptRequestHandler);
        Unreg(AdvanceStageRequestHandler);
        Unreg(PauseRequestHandler);
        Unreg(ResumeRequestHandler);

        // ---- Deferred evaluation ----
        Unreg(DeferredEvaluationRequestHandler);
        Unreg(ReplayDeferredResultRequestHandler);
        Unreg(DeferredResultReportHandler);

        Unreg(RegisterChoreActorHandler);
        Unreg(UnregisterChoreActorHandler);

        Unreg(SaveChoreActorDataHandler);
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

        // ---- НОВОЕ: остановить pulse-таймер ----
        if (AvailabilityPulseHandle.IsValid())
        {
            TimerManager->ClearTimer(AvailabilityPulseHandle);
            AvailabilityPulseHandle.Invalidate();
        }
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

// ---- Loading definitions ----
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

bool UChoreManagerSubsystem::GetChoreRewards(FName ChoreId, FChoreRewardSet& OutRewards) const
{
    OutRewards = FChoreRewardSet();

    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def) return false;

    // The author forbade showing the reward in advance — do not disclose it at all.
    // Автор запретил показывать награду заранее — не раскрываем её вообще.
    if (!Def->bShowRewardBeforeAccept) return false;

    // The chore has no rewards — nothing to return.
    // У хоры нет наград — возвращать нечего.
    const bool bHasAnyReward =
        Def->Rewards.Money != 0 ||
        Def->Rewards.Experience != 0 ||
        Def->Rewards.ItemIds.Num() > 0;
    if (!bHasAnyReward) return false;

    OutRewards = Def->Rewards;
    return true;
}

bool UChoreManagerSubsystem::CanShowRewardsBeforeAccept(FName ChoreId) const
{
    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    return Def && Def->bShowRewardBeforeAccept;
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
        NewState.CurrentStageIndex = 0;
        NewState.CurrentStageKey = Definition->Stages.IsEmpty()
            ? NAME_None
            : Definition->Stages[0].StageKey;
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

    // If a handler is already registered for this task — exit
    // Если для этого задания уже зарегистрирован обработчик — выходим
    if (ActiveStates.Contains(ChoreId) && ActiveStates[ChoreId].AvailabilityHandler.IsValid())
        return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
        return;

    // Compile the availability condition
    // Компилируем условие доступности
    Definition->AvailabilityCondition->CompileCondition();
    if (!Definition->AvailabilityCondition->GetCondition().IsValid())
        return;

    // Register a handler on the EventBus
    // Регистрируем обработчик на EventBus
    FOutcomeHandlerHandle Handle = EventBus->RegisterHandler(
        Definition->AvailabilityCondition,
        FOutcomeHandlerDelegate::CreateLambda([this, ChoreId](const FOutcomeEventBase&)
            {
                if (FChoreState* State = ActiveStates.Find(ChoreId))
                {
                    if (State->Status == EChoreStatus::Unavailable)
                    {
                        UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
                    }
                }
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

// ---- State management ----
// ---- Управление состоянием ----
void UChoreManagerSubsystem::UpdateChoreState(FName ChoreId, EChoreStatus NewStatus, bool bPublishEvent, bool bReactivated)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status == NewStatus) return;

    // При выходе из Active — сбрасываем отложенную оценку.
    if (State.Status == EChoreStatus::Active && NewStatus != EChoreStatus::Active)
    {
        State.bEvaluationPending = false;
        State.EvaluationToken = 0;
    }

    State.Status = NewStatus;

    // Publish an event to EventBus (if required)
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
        case EChoreStatus::Completed:
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
        case EChoreStatus::Abandoned:
            Event.OutcomeChore = EOutcomeChore::ChoreAbandoned;
            break;
        default:
            return;
        }

        UChoreResultPayload* Payload = EventBus->CreatePayload<UChoreResultPayload>();
        if (Payload)
        {
            Payload->ChoreId = ChoreId;
            Payload->bSucceeded = (NewStatus == EChoreStatus::Completed);
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
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return;

    // Already granted — do not grant a second time.
    // Уже выдана — второй раз не выдаём.
    if (State->bRewardIssued) return;

    // Only grant the reward for a successfully completed chore.
    // Награду выдаём только за успешно завершённую хору.
    if (State->Status != EChoreStatus::Completed) return;

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def) return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChoreRewardPayload* Payload = EventBus->CreatePayload<UChoreRewardPayload>();
    if (!Payload) return;

    Payload->Setup(ChoreId, Def->DisplayName, Def->Rewards);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Chore;
    Event.OutcomeChore = EOutcomeChore::ChoreRewardGranted;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);

    // Mark immediately — subsystems must be idempotent,
    // and a repeated call from our side will no longer pass this check.
    // Помечаем сразу — подсистемы обязаны быть идемпотентны,
    // а повторный вызов с нашей стороны уже не пройдёт эту проверку.
    State->bRewardIssued = true;

    UE_LOG(LogTemp, Log, TEXT("ChoreManager: reward issued for chore '%s' (Money=%d, Items=%d, XP=%d)"),
        *ChoreId.ToString(),
        Def->Rewards.Money,
        Def->Rewards.ItemIds.Num(),
        Def->Rewards.Experience);
}

void UChoreManagerSubsystem::AddHistoryEntry(FName ChoreId, EOutcomeChore Result, const FChorePerformanceMetrics& Performance)
{
    FChoreHistoryEntry Entry;
    Entry.ChoreId = ChoreId;
    Entry.Result = Result;
    Entry.Performance = Performance;
    Entry.Timestamp = GetGameTime();   // было FDateTime::UtcNow()
    History.Add(Entry);

    UE_LOG(LogTemp, Log, TEXT("AddHistoryEntry: History size now %d"), History.Num());

    if (History.Num() > 100)
    {
        History.RemoveAt(0, History.Num() - 100);
        UE_LOG(LogTemp, Log, TEXT("AddHistoryEntry: History trimmed to 100 entries"));
    }
}

// ---- Public management methods (called only from handlers) ----
// ---- Публичные методы управления (вызываются только из обработчиков) ----
void UChoreManagerSubsystem::OfferChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];

    // The task is already accepted or being executed — do not touch it
    // Задание уже принято или выполняется – не трогаем
    if (State.Status == EChoreStatus::Accepted || State.Status == EChoreStatus::Active)
        return;

    if (State.Status == EChoreStatus::Available || State.Status == EChoreStatus::Offered)
        return;

    if (State.Status == EChoreStatus::Completed /* && !Definitions[ChoreId]->bIsRepeatable*/)
        return;

    UpdateChoreState(ChoreId, EChoreStatus::Available);
    State.IsExpired = false;
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
    State.IsExpired = false;

    if (NewStatus == EChoreStatus::Active)
    {
        // ---- record the actual start of execution ----
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

    // ---- record the start moment ----
    // ---- фиксируем момент старта ----
    State.StartTime = FDateTime::UtcNow();

    // Recompute the deadline from the current moment (start of execution)
    // Пересчитываем дедлайн от текущего момента (начала выполнения)
    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def)
    {
        // If a deadline is set (non-zero), recompute from the current time
        // Если дедлайн задан (не нулевой), пересчитываем от текущего времени
        if (Def->Deadline != FTimespan::Zero())
        {
            State.Deadline = FDateTime::UtcNow() + Def->Deadline;
        }
        else
        {
            State.Deadline = FDateTime::MinValue(); // no deadline // без дедлайна
        }
    }

    UpdateChoreState(ChoreId, EChoreStatus::Active);
    StartDeadlineTimer(ChoreId);
    State.IsExpired = false;
}

void UChoreManagerSubsystem::CompleteChore(FName ChoreId, const FChorePerformanceMetrics& Performance)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);

    FChorePerformanceMetrics FinalPerformance = Performance;
    // If the time did not come from the payload — record the actual one.
    // Если время не пришло из payload — фиксируем фактическое.
    if (FinalPerformance.CompletionTimeSeconds <= 0.0f &&
        State.StartTime != FDateTime::MinValue())
    {
        FinalPerformance.CompletionTimeSeconds =
            (float)(FDateTime::UtcNow() - State.StartTime).GetTotalSeconds();
    }

    State.Performance = Performance;
    State.bSucceeded = true;
    AddHistoryEntry(ChoreId, EOutcomeChore::CompleteRequest, Performance);
    UpdateChoreState(ChoreId, EChoreStatus::Completed);
    GrantRewards(ChoreId);
    State.IsExpired = false;

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def /* && Def->bIsRepeatable*/)
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
    State.IsExpired = false;
    State.bSucceeded = false;

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def /* && Def->bIsRepeatable*/)
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
    State.IsExpired = true;

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def /* && Def->bIsRepeatable*/)
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
    {
        return;
    }

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);

    ClearDeadlineTimer(ChoreId);
    EnsureElapsedTimeRecorded(State);
    UpdateChoreState(ChoreId, EChoreStatus::Abandoned);
    AddHistoryEntry(ChoreId, EOutcomeChore::AbandonRequest, State.Performance);
    State.IsExpired = false;


    if (Def /* && Def->bIsRepeatable*/)
    {
        if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
        {
            UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        }
        else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional /* && Def->ReactivationCondition*/)
        {
            RegisterReactivationHandler(Def);
        }
    }
}

void UChoreManagerSubsystem::RetryChore(FName ChoreId)
{
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return;

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (Def && Def->RetryBehavior == EChoreRetryBehavior::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChoreManager: RetryChore called for chore '%s' with RetryBehavior::None"), *ChoreId.ToString());
        return;
	}

    State->IsExpired = false;

    if (State->Status == EChoreStatus::PendingReaccept || State->Status == EChoreStatus::Failed || State->Status == EChoreStatus::Abandoned)
    {
        RestartChore(ChoreId);
        State->bRewardIssued = false;
        UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        return;
    }
}

void UChoreManagerSubsystem::UnlockChore(FName ChoreId)
{
    OfferChore(ChoreId);
}

void UChoreManagerSubsystem::RevokeChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];

    State.IsExpired = false;

    // Revoke only if the task has not yet been accepted
    // Отзываем только если задание ещё не принято
    if (State.Status == EChoreStatus::Available || State.Status == EChoreStatus::Offered)
    {
        UnregisterAvailabilityHandler(ChoreId); // unsubscribe from the handler, if any // отписываемся от обработчика, если есть
        UpdateChoreState(ChoreId, EChoreStatus::Unavailable); // move to unavailable // переводим в недоступное
        UE_LOG(LogTemp, Log, TEXT("ChoreManager: Revoked chore '%s' (condition no longer met)"), *ChoreId.ToString());
    }
}

// ---- For missions ----
// ---- Для миссий ----
/*
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
*/

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

// ---- State queries (public) ----
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

FChoreState UChoreManagerSubsystem::GetState(FName ChoreId) const
{
    if (!ActiveStates.Contains(ChoreId)) return FChoreState();

    return ActiveStates[ChoreId];
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

TArray<FName> UChoreManagerSubsystem::GetAbandonedChoreIds() const
{
    TArray<FName> Result;
    for (const auto& Pair : ActiveStates)
    {
        if (Pair.Value.Status == EChoreStatus::Abandoned)
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

// ---- Execution time ----
// ---- Время выполнения ----
float UChoreManagerSubsystem::GetChoreElapsedTime(FName ChoreId) const
{
    if (!ActiveStates.Contains(ChoreId)) return 0.0f;
    const FChoreState& State = ActiveStates[ChoreId];

    if (State.Status == EChoreStatus::Active && State.StartTime != FDateTime::MinValue())
    {
        FTimespan Elapsed = FDateTime::UtcNow() - State.StartTime;

        // Subtract the accumulated pause…
        // Вычитаем накопленную паузу…
        Elapsed -= State.AccumulatedPauseTime;

        // …and the current unfinished pause, if paused right now.
        // …и текущую незавершённую паузу, если прямо сейчас на паузе.
        if (State.bIsPaused && State.PauseStartTime != FDateTime::MinValue())
        {
            Elapsed -= (FDateTime::UtcNow() - State.PauseStartTime);
        }

        if (Elapsed.GetTotalSeconds() < 0.0)
            Elapsed = FTimespan::Zero();

        return (float)Elapsed.GetTotalSeconds();
    }

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

    // Shift StartTime so that GetChoreElapsedTime() returns the desired value.
    // Сдвигаем StartTime так, чтобы GetChoreElapsedTime() вернул нужное значение.
    State.StartTime = FDateTime::UtcNow() - FTimespan::FromSeconds(ElapsedSeconds);

    // If the chore is already completed — write directly into the metrics.
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
    if (State.Performance.CompletionTimeSeconds > 0.0f) return;
    if (State.StartTime == FDateTime::MinValue()) return;

    FTimespan Elapsed = FDateTime::UtcNow() - State.StartTime;
    Elapsed -= State.AccumulatedPauseTime;
    if (State.bIsPaused && State.PauseStartTime != FDateTime::MinValue())
    {
        Elapsed -= (FDateTime::UtcNow() - State.PauseStartTime);
    }
    if (Elapsed.GetTotalSeconds() < 0.0)
        Elapsed = FTimespan::Zero();

    State.Performance.CompletionTimeSeconds = (float)Elapsed.GetTotalSeconds();
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
        if (Pair.Value.Status == EChoreStatus::Completed)
        {
            Result.Add(Pair.Key);
        }
    }
    return Result;
}

// ---- Methods for history conditions ----
// ---- Методы для условий истории ----
// ---- Extended history queries ----
// ---- Расширенные запросы истории ----

FDateTime UChoreManagerSubsystem::GetGameTime() const
{
    return FDateTime::UtcNow() - AccumulatedOfflineTime;
}

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
    // No filters — nothing to count.
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
        EOutcomeChore::Default, false);
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
    // If no filter is set, return 0
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

bool UChoreManagerSubsystem::GetLatestHistoryTimestamp(
    FName ChoreId,
    EChoreFamily Family,
    bool bUseFamily,
    EChoreSubtype Subtype,
    bool bUseSubtype,
    const TArray<EOutcomeChore>& AllowedResults,
    FDateTime& OutTimestamp) const
{
    if (ChoreId.IsNone() && !bUseFamily && !bUseSubtype)
        return false;

    for (int32 i = History.Num() - 1; i >= 0; --i)
    {
        const FChoreHistoryEntry& Entry = History[i];

        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;
        if (AllowedResults.Num() > 0 && !AllowedResults.Contains(Entry.Result)) continue;

        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family)  continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }

        OutTimestamp = Entry.Timestamp;
        return true;
    }
    return false;
}

int32 UChoreManagerSubsystem::GetChoreCurrentStage(FName ChoreId) const
{
    const FChoreState* State = ActiveStates.Find(ChoreId);
    return State ? State->CurrentStageIndex : 0;
}

int32 UChoreManagerSubsystem::GetChoreTotalStages(FName ChoreId) const
{
    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    return Def ? Def->Stages.Num() : 0;
}

FName UChoreManagerSubsystem::GetChoreCurrentStageKey(FName ChoreId) const
{
    const FChoreState* State = ActiveStates.Find(ChoreId);
    return State ? State->CurrentStageKey : NAME_None;
}

bool UChoreManagerSubsystem::IsChorePaused(FName ChoreId) const
{
    return ActiveStates.Contains(ChoreId) && ActiveStates[ChoreId].bIsPaused;
}

bool UChoreManagerSubsystem::IsChoreMultiStage(FName ChoreId) const
{
    return GetChoreTotalStages(ChoreId) > 1;
}

bool UChoreManagerSubsystem::GetChoreStageDefinition(FName ChoreId, int32 StageIndex, FChoreStageDefinition& OutStage) const
{
    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def || !Def->Stages.IsValidIndex(StageIndex)) return false;
    OutStage = Def->Stages[StageIndex];
    return true;
}

bool UChoreManagerSubsystem::GetChoreCurrentStageDefinition(FName ChoreId, FChoreStageDefinition& OutStage) const
{
    return GetChoreStageDefinition(ChoreId, GetChoreCurrentStage(ChoreId), OutStage);
}

bool UChoreManagerSubsystem::WasRewardIssued(FName ChoreId) const
{
    const FChoreState* State = ActiveStates.Find(ChoreId);
    return State ? State->bRewardIssued : false;
}

void UChoreManagerSubsystem::RequestRewardIssue(FName ChoreId)
{
    // Repeated calls are safe: GrantRewards protects itself.
    // Повторный вызов безопасен: GrantRewards сам себя защищает.
    GrantRewards(ChoreId);
}

// ---- Event handlers ----
// ---- Обработчики событий ----
void UChoreManagerSubsystem::HandleEvent(const FOutcomeEventBase& Outcome)
{
    UE_LOG(LogTemp, Log, TEXT("HandleEvent: Type=%d, Mission=%d, Chore=%d"),
        (int32)Outcome.OutcomeType, (int32)Outcome.OutcomeMission, (int32)Outcome.OutcomeChore);

    // Ignore command events (they end in "Request")
    // They must not affect task availability
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
            Chore == EOutcomeChore::UnlockRequest ||
            Chore == EOutcomeChore::DeferredEvaluationRequest ||
            Chore == EOutcomeChore::DeferredEvaluationQuery ||
            Chore == EOutcomeChore::DeferredResultReport ||
            Chore == EOutcomeChore::ReplayDeferredResultRequest ||
            Chore == EOutcomeChore::DeferredResultReplay)
        {
            return; // Command events do not affect availability // Командные события не влияют на доступность
        }
    }

    // Iterate over all tasks
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

            // Check whether the condition is event-based (should not be revoked)
            // Проверяем, является ли условие событийным (не должно отзываться)
            if (UMissionConditionAsset* MissionCond = Cast<UMissionConditionAsset>(Def->AvailabilityCondition))
            {
                // For IsCompleted, IsFailed, IsAbandoned disable revocation entirely
                // Для IsCompleted, IsFailed, IsAbandoned отключаем отзыв полностью
                if (MissionCond->ConditionType == EMissionConditionType::IsCompleted ||
                    MissionCond->ConditionType == EMissionConditionType::IsFailed ||
                    MissionCond->ConditionType == EMissionConditionType::IsAbandoned)
                {
                    bShouldRevoke = false;
                }
                // For StepReached disable revocation only for non-mission events
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

/*
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
        if (State.Status == EChoreStatus::Active || State.Status == EChoreStatus::Completed)
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
*/

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
        UpdateChoreState(ChoreId, EChoreStatus::Completed);
        GrantRewards(ChoreId);
        AddHistoryEntry(ChoreId, EOutcomeChore::CompleteRequest, Result->Performance);

        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def /* && Def->bIsRepeatable*/)
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
        if (Def /* && Def->bIsRepeatable*/)
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

// ---- Implementation of command handlers ----
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

// ---- Helper functions ----
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
        FChoreState& State = Pair.Value;
        const EChoreStatus Status = State.Status;

        // ── Conditional-реактивация (state-driven) ────────────────────────
        //
        // Хора в Completed/Failed/Expired/Abandoned с RetryBehavior=Conditional
        // и state-driven ReactivationCondition не увидит «стало true» без
        // внешнего события. Pulse — единственный, кто может это заметить.
        //
        // Event-driven условия (MissionCondition, WorldState FactAdded/…)
        // здесь не переоцениваются: их Evaluate(Dummy) вернёт false вне
        // контекста события, и мы бы ошибочно решили, что условие не
        // выполнено. Они ждут события в EventBus.
        const bool bCanBeReactivated =
            (Status == EChoreStatus::Completed ||
                Status == EChoreStatus::Failed ||
                Status == EChoreStatus::Expired ||
                Status == EChoreStatus::Abandoned);   // ← добавлено

        if (bCanBeReactivated)
        {
            UChoreDefinition* Def = GetChoreDefinition(ChoreId);
            if (Def
                && Def->RetryBehavior == EChoreRetryBehavior::Conditional
                && Def->ReactivationCondition
                && Def->ReactivationCondition->IsStateDriven())
            {
                if (!Def->ReactivationCondition->GetCondition().IsValid())
                    Def->ReactivationCondition->CompileCondition();

                if (Def->ReactivationCondition->GetCondition().IsValid())
                {
                    FOutcomeEventBase Dummy;
                    if (Def->ReactivationCondition->GetCondition()->Evaluate(Dummy))
                    {
                        UnregisterReactivationHandler(ChoreId);
                        UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
                    }
                }
            }
            // Из Completed/Failed/Expired/Abandoned availability-ветка ниже
            // ничего не делает — уходим на следующую итерацию.
            continue;
        }

        // ── Availability ──────────────────────────────────────────────────
        const bool bCanBeOffered =
            (Status == EChoreStatus::Unavailable || Status == EChoreStatus::RetryAvailable);
        const bool bCanBeRevoked =
            (Status == EChoreStatus::Available || Status == EChoreStatus::Offered);

        if (!bCanBeOffered && !bCanBeRevoked) continue;

        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (!Def || !Def->AvailabilityCondition) continue;

        if (!Def->AvailabilityCondition->GetCondition().IsValid())
            Def->AvailabilityCondition->CompileCondition();
        if (!Def->AvailabilityCondition->GetCondition().IsValid()) continue;

        FOutcomeEventBase Dummy;
        const bool bConditionMet = Def->AvailabilityCondition->GetCondition()->Evaluate(Dummy);

        if (bCanBeOffered && bConditionMet)
        {
            OfferChore(ChoreId);
        }
        else if (bCanBeRevoked && !bConditionMet)
        {
            if (Def->AvailabilityCondition->IsStateDriven())
            {
                RevokeChore(ChoreId);
            }
        }
    }
}

void UChoreManagerSubsystem::RegisterReactivationHandler(UChoreDefinition* Definition)
{
    if (!Definition || !Definition->ReactivationCondition)
        return;

    FName ChoreId = Definition->GetChoreId();

    // Если для этой хоры уже есть активный реактивационный хендлер — не плодим.
    // Раньше здесь ошибочно проверялся AvailabilityHandler, из-за чего для хор
    // с AvailabilityCondition реактивация не регистрировалась вовсе.
    if (ActiveStates.Contains(ChoreId) && ActiveStates[ChoreId].ReactivationHandler.IsValid())
        return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
        return;

    Definition->ReactivationCondition->CompileCondition();
    if (!Definition->ReactivationCondition->GetCondition().IsValid())
        return;

    // Условие уже истинно — реактивируем сразу.
    FOutcomeEventBase Dummy;
    Dummy.OutcomeType = EOutcomeType::Default;
    if (Definition->ReactivationCondition->GetCondition()->Evaluate(Dummy))
    {
        UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
        return;
    }

    // Иначе подписываемся на событие.
    FOutcomeHandlerHandle Handle = EventBus->RegisterHandler(
        Definition->ReactivationCondition,
        FOutcomeHandlerDelegate::CreateLambda([this, ChoreId](const FOutcomeEventBase&)
            {
                FChoreState* State = ActiveStates.Find(ChoreId);
                if (!State) return;

                // Реактивация осмысленна только из «завершённых» статусов:
                // Completed / Failed / Expired / Abandoned / RetryAvailable.
                // Если хора уже Accepted/Active/PendingReaccept/Available —
                // не трогаем её. Иначе state-driven условие (например,
                // Location) сработает от любого события, на котором оно
                // оказалось истинным, и откатит активную попытку в Available.
                if (State->Status != EChoreStatus::Completed &&
                    State->Status != EChoreStatus::Failed &&
                    State->Status != EChoreStatus::Expired &&
                    State->Status != EChoreStatus::Abandoned &&
                    State->Status != EChoreStatus::RetryAvailable)
                {
                    return;
                }

                // Одноразовый хендлер. Снимаем ДО изменения состояния,
                // потому что UpdateChoreState публикует события в EventBus,
                // и мы не хотим, чтобы наш же хендлер отреагировал на них.
                UnregisterReactivationHandler(ChoreId);
                UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
            })
    );

    if (Handle.IsValid())
    {
        ActiveStates[ChoreId].ReactivationHandler = Handle;
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

void UChoreManagerSubsystem::AdvanceChoreStage(FName ChoreId, int32 NewStageIndex, bool IsStart, FName NewStageKey)
{
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State || State->Status != EChoreStatus::Active) return;

    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def || !Def->Stages.IsValidIndex(NewStageIndex)) return;

    // Not backwards
    // Не назад
    if (NewStageIndex < State->CurrentStageIndex) return;

    // Take the key from the asset — the mini-game only sends the index.
    // Ключ берём из ассета — миниигра присылает только индекс.
    State->CurrentStageIndex = NewStageIndex;
    State->CurrentStageKey = Def->Stages[NewStageIndex].StageKey;
    State->IsStart = IsStart;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChoreStagePayload* Payload = EventBus->CreatePayload<UChoreStagePayload>();
    if (!Payload) return;

    Payload->ChoreId = ChoreId;
    Payload->StageIndex = NewStageIndex;
    Payload->StageKey = State->CurrentStageKey;
    Payload->TotalStages = Def->Stages.Num();
    Payload->StageDisplayName = Def->Stages[NewStageIndex].DisplayName;
    Payload->IsStart = State->IsStart;

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Chore;
    Event.OutcomeChore = EOutcomeChore::ChoreStageAdvanced;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}

void UChoreManagerSubsystem::ResetAttemptState(FChoreState& State)
{
    State.StartTime = FDateTime::MinValue();
    State.Deadline = FDateTime::MinValue();

    State.Performance = FChorePerformanceMetrics();
    State.bSucceeded = false;

    State.CurrentStageIndex = 0;
    State.CurrentStageKey = NAME_None;

    State.bIsPaused = false;
    State.PauseStartTime = FDateTime::MinValue();
    State.AccumulatedPauseTime = FTimespan::Zero();
    State.PausedDeadlineRemaining = FTimespan::Zero();

    // ---- Deferred evaluation ----
    State.bEvaluationPending = false;
    State.EvaluationToken = 0;
}

void UChoreManagerSubsystem::RestartChore(FName ChoreId)
{
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return;

    ClearDeadlineTimer(ChoreId);
    ResetAttemptState(*State);
}

void UChoreManagerSubsystem::PauseChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;
    if (State.bIsPaused) return;

    State.bIsPaused = true;
    State.PauseStartTime = FDateTime::UtcNow();

    // ---- Freeze the deadline ----
    // Stop the timer and remember the remaining time; reset the Deadline itself
    // so that save/load doesn't "inherit" an already outdated moment in time.
    // ---- Замираем дедлайн ----
    // Снимаем таймер и запоминаем остаток; сам Deadline обнуляем,
    // чтобы save/load не «унаследовал» уже устаревший момент времени.
    if (State.Deadline != FDateTime::MinValue())
    {
        State.PausedDeadlineRemaining = State.Deadline - FDateTime::UtcNow();
        if (State.PausedDeadlineRemaining.GetTotalSeconds() < 0)
            State.PausedDeadlineRemaining = FTimespan::Zero();

        ClearDeadlineTimer(ChoreId);
        State.Deadline = FDateTime::MinValue();
    }

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChorePauseReportPayload* Payload = EventBus->CreatePayload<UChorePauseReportPayload>();
    if (!Payload) return;

    Payload->Setup(ChoreId);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Chore;
    Event.OutcomeChore = EOutcomeChore::ChorePaused;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}

void UChoreManagerSubsystem::ResumeChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;
    if (!State.bIsPaused) return;

    // Accumulate the pause time (for elapsed time).
    // Накопить время паузы (для elapsed-времени).
    if (State.PauseStartTime != FDateTime::MinValue())
    {
        State.AccumulatedPauseTime += (FDateTime::UtcNow() - State.PauseStartTime);
    }
    State.PauseStartTime = FDateTime::MinValue();
    State.bIsPaused = false;

    // ---- Restore the deadline ----
    // The countdown continues from where it was stopped.
    // ---- Восстанавливаем дедлайн ----
    // Отсчёт продолжается с того места, где был остановлен.
    if (State.PausedDeadlineRemaining.GetTotalSeconds() > 0.0)
    {
        State.Deadline = FDateTime::UtcNow() + State.PausedDeadlineRemaining;
        State.PausedDeadlineRemaining = FTimespan::Zero();
        StartDeadlineTimer(ChoreId);
    }

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChorePauseReportPayload* Payload = EventBus->CreatePayload<UChorePauseReportPayload>();
    if (!Payload) return;

    Payload->Setup(ChoreId);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Chore;
    Event.OutcomeChore = EOutcomeChore::ChoreResumed;
    Event.Payload = Payload;
    EventBus->PublishOutcome(Event);
}

// ---- ISaveableSubsystem ----
void UChoreManagerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // ================================================================
    // ШАГ 1: Запрашиваем у всех живых Акторов их актуальное состояние.
    // EventBus синхронный, поэтому все ответы (SaveChoreActorData)
    // придут и запишутся в ActorCustomDataMap ДО того, как мы начнём
    // сериализацию.
    // ================================================================
    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        FOutcomeEventBase RequestEv;
        RequestEv.OutcomeType = EOutcomeType::Chore;
        RequestEv.OutcomeChore = EOutcomeChore::RequestActorDataForSave;
        // Payload не обязателен: Акторы знают свой ChoreId сами.
        EventBus->PublishOutcome(RequestEv);

        UE_LOG(LogTemp, Log, TEXT("ChoreManager: Requested actor data for save."));
    }

    // ================================================================
    // ШАГ 2: Сериализуем актуальный ActorCustomDataMap
    // ================================================================
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

        // Дедлайн сохраняем как ОСТАТОК в секундах, а не как абсолютную метку.
        {
            double DeadlineRemainingSecs = 0.0;
            if (State.Deadline != FDateTime::MinValue())
            {
                DeadlineRemainingSecs = (State.Deadline - FDateTime::UtcNow()).GetTotalSeconds();
                if (DeadlineRemainingSecs < 0.0)
                {
                    DeadlineRemainingSecs = 0.0;
                }
            }
            Obj->SetNumberField(TEXT("DeadlineRemainingSeconds"), DeadlineRemainingSecs);
        }

        Obj->SetNumberField(TEXT("AttemptCount"), State.AttemptCount);
        Obj->SetBoolField(TEXT("bSucceeded"), State.bSucceeded);
        Obj->SetBoolField(TEXT("bRewardIssued"), State.bRewardIssued);

        // Stages
        Obj->SetNumberField(TEXT("CurrentStageIndex"), State.CurrentStageIndex);
        Obj->SetStringField(TEXT("CurrentStageKey"), State.CurrentStageKey.ToString());
        Obj->SetBoolField(TEXT("IsStart"), State.IsStart);
        Obj->SetBoolField(TEXT("IsExpired"), State.IsExpired);

        // Pause
        Obj->SetBoolField(TEXT("bIsPaused"), State.bIsPaused);
        Obj->SetNumberField(TEXT("AccumulatedPauseSeconds"), State.AccumulatedPauseTime.GetTotalSeconds());
        Obj->SetNumberField(TEXT("PausedDeadlineSeconds"), State.PausedDeadlineRemaining.GetTotalSeconds());

        // Performance
        TSharedPtr<FJsonObject> PerfObj = MakeShared<FJsonObject>();
        PerfObj->SetNumberField(TEXT("CompletionTimeSeconds"), State.Performance.CompletionTimeSeconds);
        PerfObj->SetNumberField(TEXT("Mistakes"), State.Performance.Mistakes);
        PerfObj->SetNumberField(TEXT("Accuracy"), State.Performance.Accuracy);
        PerfObj->SetNumberField(TEXT("Quantity"), State.Performance.Quantity);
        Obj->SetObjectField(TEXT("Performance"), PerfObj);

        // ---- Deferred evaluation ----
        Obj->SetBoolField(TEXT("bEvaluationPending"), State.bEvaluationPending);
        Obj->SetNumberField(TEXT("EvaluationToken"), State.EvaluationToken);

        // ---- Stored Result (корзина сдачи) ----
        Obj->SetBoolField(TEXT("bHasStoredResult"), State.bHasStoredResult);
        Obj->SetBoolField(TEXT("StoredResultSucceeded"), State.StoredResultSucceeded);

        TSharedPtr<FJsonObject> StoredPerfObj = MakeShared<FJsonObject>();
        StoredPerfObj->SetNumberField(TEXT("CompletionTimeSeconds"), State.StoredResultPerformance.CompletionTimeSeconds);
        StoredPerfObj->SetNumberField(TEXT("Mistakes"), State.StoredResultPerformance.Mistakes);
        StoredPerfObj->SetNumberField(TEXT("Accuracy"), State.StoredResultPerformance.Accuracy);
        StoredPerfObj->SetNumberField(TEXT("Quantity"), State.StoredResultPerformance.Quantity);
        Obj->SetObjectField(TEXT("StoredResultPerformance"), StoredPerfObj);

        // ---- Actor Custom Data (JSON Map) ----
        TSharedPtr<FJsonObject> DataMapObj = MakeShared<FJsonObject>();
        for (const auto& DataPair : State.ActorCustomDataMap)
        {
            DataMapObj->SetStringField(DataPair.Key.ToString(), DataPair.Value);
        }
        Obj->SetObjectField(TEXT("ActorCustomDataMap"), DataMapObj);

        StateArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("States"), StateArray);

    // ---- History ----
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

    // ---- offline-время ----
    Root->SetStringField(TEXT("LastSaveWallClock"), FDateTime::UtcNow().ToIso8601());
    Root->SetNumberField(TEXT("AccumulatedOfflineSeconds"), AccumulatedOfflineTime.GetTotalSeconds());

    // ---- Сериализация ----
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

    // ---- offline-время ----
    double AccumulatedOfflineSecs = 0.0;
    Root->TryGetNumberField(TEXT("AccumulatedOfflineSeconds"), AccumulatedOfflineSecs);
    AccumulatedOfflineTime = FTimespan::FromSeconds(FMath::Max(0.0, AccumulatedOfflineSecs));

    FString LastSaveWallClockStr;
    if (Root->TryGetStringField(TEXT("LastSaveWallClock"), LastSaveWallClockStr))
    {
        FDateTime LastSaveWallClock;
        if (FDateTime::ParseIso8601(*LastSaveWallClockStr, LastSaveWallClock))
        {
            const FTimespan Gap = FDateTime::UtcNow() - LastSaveWallClock;
            if (Gap.GetTotalSeconds() > 0)
            {
                AccumulatedOfflineTime += Gap;
            }
        }
    }

    // ---- States ----
    const TArray<TSharedPtr<FJsonValue>>* StateArray = nullptr;
    if (Root->TryGetArrayField(TEXT("States"), StateArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *StateArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FChoreState State;

            FString ChoreIdStr;
            Obj->TryGetStringField(TEXT("ChoreId"), ChoreIdStr);
            State.ChoreId = FName(*ChoreIdStr);

            int32 StatusInt = 0;
            Obj->TryGetNumberField(TEXT("Status"), StatusInt);
            State.Status = (EChoreStatus)StatusInt;

            FString AcceptTimeStr;
            Obj->TryGetStringField(TEXT("AcceptTime"), AcceptTimeStr);
            FDateTime::ParseIso8601(*AcceptTimeStr, State.AcceptTime);

            FString StartTimeStr;
            Obj->TryGetStringField(TEXT("StartTime"), StartTimeStr);
            if (!FDateTime::ParseIso8601(*StartTimeStr, State.StartTime))
            {
                State.StartTime = FDateTime::MinValue();
            }

            double DeadlineRemainingSecs = 0.0;
            Obj->TryGetNumberField(TEXT("DeadlineRemainingSeconds"), DeadlineRemainingSecs);

            if (DeadlineRemainingSecs > 0.0)
            {
                State.Deadline = FDateTime::UtcNow() + FTimespan::FromSeconds(DeadlineRemainingSecs);
            }
            else
            {
                State.Deadline = FDateTime::MinValue();
            }

            Obj->TryGetNumberField(TEXT("AttemptCount"), State.AttemptCount);
            Obj->TryGetBoolField(TEXT("bSucceeded"), State.bSucceeded);
            Obj->TryGetBoolField(TEXT("bRewardIssued"), State.bRewardIssued);

            // ---- Stages ----
            int32 CurrentStageInt = 0;
            Obj->TryGetNumberField(TEXT("CurrentStageIndex"), CurrentStageInt);
            State.CurrentStageIndex = CurrentStageInt;

            FString StageKeyStr;
            Obj->TryGetStringField(TEXT("CurrentStageKey"), StageKeyStr);
            State.CurrentStageKey = FName(*StageKeyStr);

            Obj->TryGetBoolField(TEXT("IsStart"), State.IsStart);
            Obj->TryGetBoolField(TEXT("IsExpired"), State.IsExpired);

            // ---- Pause ----
            Obj->TryGetBoolField(TEXT("bIsPaused"), State.bIsPaused);

            double AccumulatedPauseSecs = 0.0;
            Obj->TryGetNumberField(TEXT("AccumulatedPauseSeconds"), AccumulatedPauseSecs);
            State.AccumulatedPauseTime = FTimespan::FromSeconds(AccumulatedPauseSecs);

            double PausedDeadlineSecs = 0.0;
            Obj->TryGetNumberField(TEXT("PausedDeadlineSeconds"), PausedDeadlineSecs);
            State.PausedDeadlineRemaining = FTimespan::FromSeconds(PausedDeadlineSecs);

            State.PauseStartTime = State.bIsPaused ? FDateTime::UtcNow() : FDateTime::MinValue();

            // ---- Performance ----
            const TSharedPtr<FJsonObject>* PerfObj = nullptr;
            if (Obj->TryGetObjectField(TEXT("Performance"), PerfObj))
            {
                (*PerfObj)->TryGetNumberField(TEXT("CompletionTimeSeconds"), State.Performance.CompletionTimeSeconds);
                (*PerfObj)->TryGetNumberField(TEXT("Mistakes"), State.Performance.Mistakes);
                (*PerfObj)->TryGetNumberField(TEXT("Accuracy"), State.Performance.Accuracy);
                (*PerfObj)->TryGetNumberField(TEXT("Quantity"), State.Performance.Quantity);
            }

            // ---- Deferred evaluation ----
            Obj->TryGetBoolField(TEXT("bEvaluationPending"), State.bEvaluationPending);
            Obj->TryGetNumberField(TEXT("EvaluationToken"), State.EvaluationToken);

            // На момент загрузки акторы ещё не заспавнились, следовательно,
            // отвечать на запросы некому. Сбрасываем ожидание.
            State.bEvaluationPending = false;
            State.EvaluationToken = 0;

            // ---- Stored Result (корзина сдачи) ----
            Obj->TryGetBoolField(TEXT("bHasStoredResult"), State.bHasStoredResult);
            Obj->TryGetBoolField(TEXT("StoredResultSucceeded"), State.StoredResultSucceeded);

            const TSharedPtr<FJsonObject>* StoredPerfObj = nullptr;
            if (Obj->TryGetObjectField(TEXT("StoredResultPerformance"), StoredPerfObj))
            {
                (*StoredPerfObj)->TryGetNumberField(TEXT("CompletionTimeSeconds"), State.StoredResultPerformance.CompletionTimeSeconds);
                (*StoredPerfObj)->TryGetNumberField(TEXT("Mistakes"), State.StoredResultPerformance.Mistakes);
                (*StoredPerfObj)->TryGetNumberField(TEXT("Accuracy"), State.StoredResultPerformance.Accuracy);
                (*StoredPerfObj)->TryGetNumberField(TEXT("Quantity"), State.StoredResultPerformance.Quantity);
            }

            // ---- Actor Custom Data (JSON Map) ----
            const TSharedPtr<FJsonObject>* DataMapObj = nullptr;
            if (Obj->TryGetObjectField(TEXT("ActorCustomDataMap"), DataMapObj))
            {
                for (const auto& DataPair : (*DataMapObj)->Values)
                {
                    FString JsonStr;
                    if (DataPair.Value->TryGetString(JsonStr))
                    {
                        State.ActorCustomDataMap.Add(FName(*DataPair.Key), JsonStr);
                    }
                }
            }

            // При загрузке актор физически отсутствует на сцене. 
            // Флаг bIsActorAlive станет true, когда актор заспавнится и пришлет RegisterChoreActor.
            State.bIsActorAlive = false;

            ActiveStates.Add(State.ChoreId, State);

            if (State.Status == EChoreStatus::Active && State.Deadline != FDateTime::MinValue())
            {
                FTimespan Remaining = State.Deadline - FDateTime::UtcNow();
                if (Remaining.GetTotalSeconds() > 0)
                {
                    FTimerHandle Handle;
                    TimerManager->SetTimer(
                        Handle,
                        FTimerDelegate::CreateUObject(this, &UChoreManagerSubsystem::ExpireChore, State.ChoreId),
                        (float)Remaining.GetTotalSeconds(),
                        false);
                    DeadlineTimers.Add(State.ChoreId, Handle);
                }
                else
                {
                    ExpireChore(State.ChoreId);
                }
            }
        }
    }

    // ---- History ----
    const TArray<TSharedPtr<FJsonValue>>* HistoryArray = nullptr;
    if (Root->TryGetArrayField(TEXT("History"), HistoryArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *HistoryArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FChoreHistoryEntry Entry;

            FString ChoreIdStr;
            Obj->TryGetStringField(TEXT("ChoreId"), ChoreIdStr);
            Entry.ChoreId = FName(*ChoreIdStr);

            int32 ResultInt = 0;
            Obj->TryGetNumberField(TEXT("Result"), ResultInt);
            Entry.Result = (EOutcomeChore)ResultInt;

            FString TimestampStr;
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

    // Check whether the task exists
    // Проверяем, существует ли задание
    if (!Definitions.Contains(ChoreId))
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreRequest: chore '%s' not found"), *ChoreId.ToString());
        return;
    }

    // Check the status
    // Проверяем статус
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (State)
    {
        EChoreStatus Status = State->Status;
        // Allow removal if the task is completed, expired, unavailable, or forced
        // Разрешаем удаление, если задание завершено, истекло, недоступно или принудительно
        bool bCanRemove = (Status == EChoreStatus::Unavailable) ||
            (Status == EChoreStatus::Expired) ||
            (Status == EChoreStatus::Completed) ||
            (Status == EChoreStatus::Failed) ||
            Payload->bForceRemove;

        if (!bCanRemove)
        {
            UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreRequest: chore '%s' is in status %d and cannot be removed (use bForceRemove=true to override)"),
                *ChoreId.ToString(), (int32)Status);
            return;
        }

        // If forcibly removing an active task – complete it
        // Если принудительно удаляем активное задание – завершаем его
        if (Payload->bForceRemove && (Status == EChoreStatus::Accepted || Status == EChoreStatus::Active || Status == EChoreStatus::WaitingToStart))
        {
            if (Status == EChoreStatus::Active)
            {
                ClearDeadlineTimer(ChoreId);
            }
            State->bSucceeded = false;
            AddHistoryEntry(ChoreId, EOutcomeChore::FailRequest, State->Performance);
            UpdateChoreState(ChoreId, EChoreStatus::Failed);
        }
    }

    // Remove from Definitions and ActiveStates
    // Удаляем из Definitions и ActiveStates
    Definitions.Remove(ChoreId);
    if (State)
    {
        ActiveStates.Remove(ChoreId);
    }

    // Unsubscribe the availability handler, if any
    // Отписываем обработчик доступности, если есть
    UnregisterAvailabilityHandler(ChoreId);

    // Unsubscribe the reactivation handler, if any
    // Отписываем реактивационный обработчик, если есть.
    // Раньше этого не делалось — хендлер оставался в EventBus и
    // срабатывал на события, хотя хоры уже нет в реестре.
    UnregisterReactivationHandler(ChoreId);

    // Stop the timer, if it is still hanging (just in case)
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

    // Reactivate the task
    // Реактивируем задание
    UpdateChoreState(ChoreId, EChoreStatus::Available, true, true);
}

void UChoreManagerSubsystem::HandleAdvanceStageRequest(const FOutcomeEventBase& Outcome)
{
    UChoreStagePayload* Payload = Cast<UChoreStagePayload>(Outcome.Payload);
    if (!Payload) return;

    AdvanceChoreStage(Payload->ChoreId, Payload->StageIndex, Payload->IsStart, NAME_None);
}

void UChoreManagerSubsystem::HandlePauseRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    PauseChore(Payload->ChoreId);
}

void UChoreManagerSubsystem::HandleResumeRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;
    ResumeChore(Payload->ChoreId);
}

// ============================================================================
// Deferred evaluation — handlers
// ============================================================================

void UChoreManagerSubsystem::HandleDeferredEvaluationRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;

    FName ChoreId = Payload->ChoreId;
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State || (State->Status != EChoreStatus::Active && State->Status != EChoreStatus::Completed))
    {
        return;
    }

    UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def || !Def->bUseDeferredEvaluation) return;

    // 1. ПРИОРИТЕТ: Актор на этой же сцене -> Спрашиваем его (самые свежие данные)
    if (State->bIsActorAlive)
    {
        UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
        if (!EventBus) return;

        const int32 Token = NextEvaluationToken++;
        UChoreDeferredQueryPayload* Query = EventBus->CreatePayload<UChoreDeferredQueryPayload>();
        if (!Query) return;
        Query->Setup(ChoreId, Token);

        State->bEvaluationPending = true;
        State->EvaluationToken = Token;

        // Если был старый результат (например, пришли с другой сцены), сбрасываем его,
        // так как мы сейчас запрашиваем актуальные данные у живого Актора.
        State->bHasStoredResult = false;

        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;
        Event.OutcomeChore = EOutcomeChore::DeferredEvaluationQuery;
        Event.Payload = Query;
        EventBus->PublishOutcome(Event);

        UE_LOG(LogTemp, Log, TEXT("DeferredEval: Actor is ALIVE for '%s'. Sending Query."), *ChoreId.ToString());
        return;
    }

    // 2. Актор мертв (другая сцена), но есть сохраненный результат -> ЗАВЕРШАЕМ!
    if (State->bHasStoredResult)
    {
        UE_LOG(LogTemp, Log, TEXT("DeferredEval: Found Stored Result for '%s'. Completing now!"), *ChoreId.ToString());
        State->bHasStoredResult = false;
        if (State->StoredResultSucceeded) CompleteChore(ChoreId, State->StoredResultPerformance);
        else                              FailChore(ChoreId, State->StoredResultPerformance);
        return;
    }

    // 3. Актора нет, корзина пуста -> НИЧЕГО НЕ ДЕЛАЕМ (Пункт 7)
    UE_LOG(LogTemp, Warning, TEXT("DeferredEval: Actor is DEAD and NO Stored Result for '%s'. Doing nothing."), *ChoreId.ToString());
}

void UChoreManagerSubsystem::HandleReplayDeferredResultRequest(const FOutcomeEventBase& Outcome)
{
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload) return;

    FName ChoreId = Payload->ChoreId;
    const FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return;

    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def || !Def->bUseDeferredEvaluation) return;

    // Replay имеет смысл только для уже оценённой хоры.
    if (State->Status != EChoreStatus::Completed && State->Status != EChoreStatus::Failed)
        return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UChoreDeferredResultPayload* Report = EventBus->CreatePayload<UChoreDeferredResultPayload>();
    if (!Report) return;
    Report->Setup(ChoreId,
        State->Status == EChoreStatus::Completed,
        State->Performance);

    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Chore;
    Event.OutcomeChore = EOutcomeChore::DeferredResultReplay;
    Event.Payload = Report;
    EventBus->PublishOutcome(Event);
}

void UChoreManagerSubsystem::HandleDeferredResultReport(const FOutcomeEventBase& Outcome)
{
    UChoreDeferredResultPayload* Payload = Cast<UChoreDeferredResultPayload>(Outcome.Payload);
    if (!Payload) return;

    FName ChoreId = Payload->ChoreId;
    FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return;

    // Если это ответ на запрос от терминала (на той же сцене)
    if (Payload->Token != 0)
    {
        if (State->bEvaluationPending && Payload->Token == State->EvaluationToken)
        {
            State->bEvaluationPending = false;
            State->EvaluationToken = 0;

            // Завершаем сразу, так как терминал уже запросил
            if (Payload->bSucceeded) CompleteChore(ChoreId, Payload->Performance);
            else                     FailChore(ChoreId, Payload->Performance);
        }
    }
    // Если это отчет из EndPlay (актор уходит со сцены)
    else if (Payload->bFromEndPlay)
    {
        // Просто складываем в корзину. Ждем терминала.
        State->bHasStoredResult = true;
        State->StoredResultSucceeded = Payload->bSucceeded;
        State->StoredResultPerformance = Payload->Performance;

        State->bEvaluationPending = false;
        State->EvaluationToken = 0;

        UE_LOG(LogTemp, Log, TEXT("ChoreManager: Stored result for '%s' from EndPlay. Waiting for terminal."), *ChoreId.ToString());
    }
}

void UChoreManagerSubsystem::HandleSaveChoreActorData(const FOutcomeEventBase& Outcome)
{
    UChoreActorDataPayload* Payload = Cast<UChoreActorDataPayload>(Outcome.Payload);
    if (!Payload) return;

    FChoreState* State = ActiveStates.Find(Payload->ChoreId);
    if (!State) return;

    // Сохраняем JSON по ключу конкретного актора
    State->ActorCustomDataMap.Add(Payload->DataKey, Payload->DataJson);

    UE_LOG(LogTemp, Log, TEXT("ChoreManager: Saved actor data for '%s', ActorKey: '%s' (Length: %d)"),
        *Payload->ChoreId.ToString(), *Payload->DataKey.ToString(), Payload->DataJson.Len());
}

bool UChoreManagerSubsystem::GetChoreActorData(FName ChoreId, FName DataKey, FString& OutDataJson) const
{
    OutDataJson.Empty();
    const FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return false;

    // Ищем данные по ключу актора
    const FString* FoundJson = State->ActorCustomDataMap.Find(DataKey);
    if (FoundJson && !FoundJson->IsEmpty())
    {
        OutDataJson = *FoundJson;
        return true;
    }

    return false;
}

void UChoreManagerSubsystem::HandleRegisterChoreActor(const FOutcomeEventBase& Outcome)
{
    if (auto* Payload = Cast<UChoreCommandPayload>(Outcome.Payload))
    {
        if (auto* State = ActiveStates.Find(Payload->ChoreId))
        {
            State->bIsActorAlive = true;
        }
    }
}

void UChoreManagerSubsystem::HandleUnregisterChoreActor(const FOutcomeEventBase& Outcome)
{
    // 1. Проверяем, что пришел правильный тип пейлоада
    UChoreCommandPayload* Payload = Cast<UChoreCommandPayload>(Outcome.Payload);
    if (!Payload)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreActor: Invalid payload type!"));
        return;
    }

    // 2. Ищем состояние хоры по её ID
    FChoreState* State = ActiveStates.Find(Payload->ChoreId);
    if (!State)
    {
        UE_LOG(LogTemp, Warning, TEXT("HandleUnregisterChoreActor: State not found for ChoreId '%s'"), *Payload->ChoreId.ToString());
        return;
    }

    // 3. Сбрасываем флаг жизни актора
    State->bIsActorAlive = false;

    // 4. Пишем лог для отладки
    UE_LOG(LogTemp, Log, TEXT("ChoreManager: Actor for chore '%s' unregistered (left scene)."), *Payload->ChoreId.ToString());
}
// ============================================================================
// Deferred evaluation — public getter
// ============================================================================

bool UChoreManagerSubsystem::GetDeferredResultForChore(
    FName ChoreId, bool& bOutSucceeded, FChorePerformanceMetrics& OutPerformance) const
{
    bOutSucceeded = false;
    OutPerformance = FChorePerformanceMetrics();

    const FChoreState* State = ActiveStates.Find(ChoreId);
    if (!State) return false;

    const UChoreDefinition* Def = GetChoreDefinition(ChoreId);
    if (!Def || !Def->bUseDeferredEvaluation) return false;

    if (State->Status != EChoreStatus::Completed && State->Status != EChoreStatus::Failed)
        return false;

    bOutSucceeded = (State->Status == EChoreStatus::Completed);
    OutPerformance = State->Performance;
    return true;
}
