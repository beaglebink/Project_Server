#include "ChoreManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "JsonObjectConverter.h"
#include "ChoreHistoryConditionAsset.h"
#include "CheckRequestPayload.h"
#include "CheckResponsePayload.h"
#include "SaveGame/GameSaveSubsystem.h"

void UChoreManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    TimerManager = &GetWorld()->GetTimerManager();

    // ---- Загрузка всех определений хор (можно через Primary Asset Manager) ----
    LoadAllDefinitions();

    // ---- Создание условий для подписок ----
    // 1. Глобальный обработчик для проверки доступности (все события)
    GlobalEventCondition = NewObject<UOutcomeConditionAsset>(this);
    GlobalEventCondition->OperatorType = EConditionOperator::Composite;
    GlobalEventCondition->FilterRow.OutcomeType = EOutcomeType::Default; // все типы
    GlobalEventCondition->CompileCondition();

    // 2. Запросы от миссий на выполнение хоры как шага (используем новый тип EOutcomeMission::ChoreStepRequest)
    MissionRequestCondition = NewObject<UOutcomeConditionAsset>(this);
    MissionRequestCondition->OperatorType = EConditionOperator::Composite;
    MissionRequestCondition->FilterRow.OutcomeType = EOutcomeType::Mission;
    MissionRequestCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    MissionRequestCondition->FilterRow.MissionType = EOutcomeMission::ChoreStepRequest;
    MissionRequestCondition->FilterRow.MissionComparison = EConditionComparison::Equals;
    MissionRequestCondition->CompileCondition();

    // 3. Завершение хоры (публикуется мини-игрой или самой хорой)
    //    Используется для обработки обычных (не миссионных) хор.
    ChoreCompletionCondition = NewObject<UOutcomeConditionAsset>(this);
    ChoreCompletionCondition->OperatorType = EConditionOperator::Composite;
    ChoreCompletionCondition->FilterRow.OutcomeType = EOutcomeType::Chore;
    ChoreCompletionCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    // Обрабатываем как успех, так и провал – поэтому фильтруем по OutcomeChore Succeeded/Failed
    // Можно сделать два отдельных обработчика, но для простоты используем один и внутри проверяем.
    ChoreCompletionCondition->CompileCondition();

    // 4. CheckRequest для запросов состояния (опционально)
    /*
    CheckRequestCondition = NewObject<UOutcomeConditionAsset>(this);
    CheckRequestCondition->OperatorType = EConditionOperator::Composite;
    CheckRequestCondition->FilterRow.OutcomeType = EOutcomeType::Chore;
    CheckRequestCondition->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    CheckRequestCondition->FilterRow.ChoreType = EOutcomeChore::CheckRequest;
    CheckRequestCondition->FilterRow.ChoreComparison = EConditionComparison::Equals;
    CheckRequestCondition->CompileCondition();
    */

    // ---- Регистрация обработчиков в EventBus ----
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        // Глобальный обработчик для условий доступности
        GlobalEventHandler = EventBus->RegisterHandler(
            GlobalEventCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleEvent)
        );

        // Обработчик запросов от миссий
        MissionRequestHandler = EventBus->RegisterHandler(
            MissionRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleMissionRequest)
        );

        // Обработчик завершения хор (обычных)
        ChoreCompletionHandler = EventBus->RegisterHandler(
            ChoreCompletionCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleChoreCompletion)
        );

        // Обработчик проверочных запросов
        /*
        CheckRequestHandler = EventBus->RegisterHandler(
            CheckRequestCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UChoreManagerSubsystem::HandleCheckRequest)
        );
        */
    }

    // ---- Регистрация в системе сохранения ----
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    UE_LOG(LogTemp, Log, TEXT("ChoreManagerSubsystem: Initialized with EventBus handlers."));
}

void UChoreManagerSubsystem::Deinitialize()
{
    // Отписываемся
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        if (GlobalEventHandler.IsValid()) EventBus->UnregisterHandler(GlobalEventHandler);
        if (MissionRequestHandler.IsValid()) EventBus->UnregisterHandler(MissionRequestHandler);
        if (ChoreCompletionHandler.IsValid()) EventBus->UnregisterHandler(ChoreCompletionHandler);
        //if (CheckRequestHandler.IsValid()) EventBus->UnregisterHandler(CheckRequestHandler);
    }

    // Отписываем все условия доступности
    for (auto& Pair : ActiveStates)
    {
        UnregisterAvailabilityHandler(Pair.Key);
    }

    // Очищаем таймеры
    for (auto& Pair : DeadlineTimers)
    {
        if (Pair.Value.IsValid()) TimerManager->ClearTimer(Pair.Value);
    }
    DeadlineTimers.Empty();

    // Отписываемся от сохранения
    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->UnregisterSaveableSubsystem(this);
    }

    Super::Deinitialize();
}

// ---- Загрузка определений ----
void UChoreManagerSubsystem::LoadAllDefinitions()
{
    // Используем Primary Asset Manager для загрузки всех UChoreDefinition
    // Либо просто сканируем папки. Здесь упрощённый вариант: вручную добавим в будущем.
    // Можно сделать метод RegisterChoreDefinition, который будет вызываться извне.
}

void UChoreManagerSubsystem::RegisterChoreDefinition(UChoreDefinition* Definition)
{
    if (!Definition) return;
    FName ChoreId = Definition->GetChoreId();
    if (Definitions.Contains(ChoreId)) return;

    Definitions.Add(ChoreId, Definition);
    // Создаём состояние, если его нет
    if (!ActiveStates.Contains(ChoreId))
    {
        FChoreState NewState;
        NewState.ChoreId = ChoreId;
        NewState.Status = EChoreStatus::Unavailable;
        ActiveStates.Add(ChoreId, NewState);
    }

    // Регистрируем обработчик условия доступности
    RegisterAvailabilityHandler(Definition);

    // Если условие уже выполнено (например, при загрузке), сразу делаем доступным
    if (Definition->AvailabilityCondition)
    {
        // Проверим условие без события – можно вызвать Evaluate с пустым событием или просто через HandleEvent
        // Для простоты вызовем EvaluateAllAvailability после регистрации всех определений.
    }
}

void UChoreManagerSubsystem::RegisterAvailabilityHandler(UChoreDefinition* Definition)
{
    if (!Definition || !Definition->AvailabilityCondition) return;
    FName ChoreId = Definition->GetChoreId();
    if (ActiveStates.Contains(ChoreId) && ActiveStates[ChoreId].AvailabilityHandler.IsValid())
        return;

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    // Компилируем условие
    Definition->AvailabilityCondition->CompileCondition();
    if (!Definition->AvailabilityCondition->GetCondition().IsValid())
        return;

    // Создаём обработчик, который при выполнении условия вызовет OfferChore
    FOutcomeHandlerHandle Handle = EventBus->RegisterHandler(
        Definition->AvailabilityCondition,
        FOutcomeHandlerDelegate::CreateLambda([this, ChoreId](const FOutcomeEventBase&)
        {
            OfferChore(ChoreId);
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
void UChoreManagerSubsystem::UpdateChoreState(FName ChoreId, EChoreStatus NewStatus, bool bPublishEvent)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    State.Status = NewStatus;

    if (bPublishEvent)
    {
        UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
        if (!EventBus) return;

        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;
        Event.OutcomeChore = EOutcomeChore::Default;
        switch (NewStatus)
        {
            case EChoreStatus::Available:
            case EChoreStatus::Offered:
                Event.OutcomeChore = EOutcomeChore::ChoreOffered;
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
            default:
                return;
        }

        // Добавим payload с состоянием (опционально)
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

    // Удаляем старый таймер
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

void UChoreManagerSubsystem::AddHistoryEntry(FName ChoreId, bool bSucceeded, const FChorePerformanceMetrics& Performance)
{
    FChoreHistoryEntry Entry;
    Entry.ChoreId = ChoreId;
    Entry.bSucceeded = bSucceeded;
    Entry.Performance = Performance;
    Entry.Timestamp = FDateTime::UtcNow();
    History.Add(Entry);

    // Ограничим размер истории, например 100 записей
    if (History.Num() > 100)
    {
        History.RemoveAt(0, History.Num() - 100);
    }
}

// ---- Публичные методы ----
void UChoreManagerSubsystem::OfferChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
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
        State.Deadline = Def->Deadline.GetTicks() > 0 ? State.AcceptTime + Def->Deadline : FDateTime::MinValue();
        State.AttemptCount++;
    }

    EChoreStatus NewStatus = (Def && Def->bMustStartImmediately) ? EChoreStatus::Active : EChoreStatus::Accepted;
    UpdateChoreState(ChoreId, NewStatus);

    if (NewStatus == EChoreStatus::Active)
    {
        StartDeadlineTimer(ChoreId);
    }
}

void UChoreManagerSubsystem::StartChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Accepted && State.Status != EChoreStatus::WaitingToStart)
        return;

    UpdateChoreState(ChoreId, EChoreStatus::Active);
    StartDeadlineTimer(ChoreId);
}

void UChoreManagerSubsystem::CompleteChore(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);
    State.Performance = Performance;
    State.bSucceeded = bSuccess;

    if (bSuccess)
    {
        UpdateChoreState(ChoreId, EChoreStatus::Succeeded);
        GrantRewards(ChoreId);
        AddHistoryEntry(ChoreId, true, Performance);
        // Проверяем повторяемость
        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def && Def->bIsRepeatable)
        {
            if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
            {
                UpdateChoreState(ChoreId, EChoreStatus::RetryAvailable);
            }
            else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
            {
                // Регистрируем обработчик реактивации
                RegisterAvailabilityHandler(Def); // переиспользуем
            }
        }
    }
    else
    {
        UpdateChoreState(ChoreId, EChoreStatus::Failed);
        AddHistoryEntry(ChoreId, false, Performance);
        // Обработка повтора
        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def && Def->RetryBehavior == EChoreRetryBehavior::Immediate)
        {
            UpdateChoreState(ChoreId, EChoreStatus::RetryAvailable);
        }
        else if (Def && Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
        {
            RegisterAvailabilityHandler(Def);
        }
    }
}

void UChoreManagerSubsystem::FailChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);
    State.bSucceeded = false;
    UpdateChoreState(ChoreId, EChoreStatus::Failed);
    AddHistoryEntry(ChoreId, false, State.Performance);
}

void UChoreManagerSubsystem::ExpireChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Active) return;

    ClearDeadlineTimer(ChoreId);
    State.bSucceeded = false;
    UpdateChoreState(ChoreId, EChoreStatus::Expired);
    AddHistoryEntry(ChoreId, false, State.Performance);
}

void UChoreManagerSubsystem::AbandonChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::Accepted && State.Status != EChoreStatus::WaitingToStart && State.Status != EChoreStatus::Active)
        return;

    ClearDeadlineTimer(ChoreId);
    UpdateChoreState(ChoreId, EChoreStatus::Failed);
    AddHistoryEntry(ChoreId, false, State.Performance);
}

void UChoreManagerSubsystem::RetryChore(FName ChoreId)
{
    if (!ActiveStates.Contains(ChoreId)) return;
    FChoreState& State = ActiveStates[ChoreId];
    if (State.Status != EChoreStatus::RetryAvailable) return;

    // Сбрасываем состояние и предлагаем снова
    State.bRewardIssued = false;
    State.Performance = FChorePerformanceMetrics();
    UpdateChoreState(ChoreId, EChoreStatus::Available);
}

void UChoreManagerSubsystem::UnlockChore(FName ChoreId)
{
    // Принудительно делаем доступным (используется внешними системами)
    OfferChore(ChoreId);
}

void UChoreManagerSubsystem::RequestMissionChore(FName MissionId, FName ChoreId, int32 StepIndex)
{
    // Этот метод вызывается миссией, когда она хочет запустить хору как шаг.
    // Он публикует событие, на которое подписан ChoreManager (HandleMissionRequest).

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
    {
        UE_LOG(LogTemp, Error, TEXT("ChoreManager: RequestMissionChore - EventBus not available"));
        return;
    }

    // Создаём payload с идентификаторами
    UChoreMissionRequestPayload* Payload = EventBus->CreatePayload<UChoreMissionRequestPayload>();
    if (!Payload)
    {
        UE_LOG(LogTemp, Error, TEXT("ChoreManager: RequestMissionChore - failed to create payload"));
        return;
    }

    Payload->MissionId = MissionId;
    Payload->ChoreId = ChoreId;
    Payload->MissionStepIndex = StepIndex;

    // Публикуем событие типа Mission / ChoreStepRequest
    FOutcomeEventBase Event;
    Event.OutcomeType = EOutcomeType::Mission;
    Event.OutcomeMission = EOutcomeMission::ChoreStepRequest;
    Event.Payload = Payload;

    EventBus->PublishOutcome(Event);

    UE_LOG(LogTemp, Log,
        TEXT("ChoreManager: RequestMissionChore - published request for Mission='%s', Chore='%s', Step=%d"),
        *MissionId.ToString(), *ChoreId.ToString(), StepIndex);
}

void UChoreManagerSubsystem::ReportMissionChoreResult(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance, FName MissionId)
{
    // Записываем в историю, не меняя статус задания
    AddHistoryEntry(ChoreId, bSuccess, Performance);

    // Публикуем результат для миссии
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

// ---- Запросы состояния ----
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

// ---- Для условий истории ----
int32 UChoreManagerSubsystem::GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const
{
    int32 Count = 0;
    for (const FChoreHistoryEntry& Entry : History)
    {
        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;
        if (bSucceededOnly && !Entry.bSucceeded) continue;

        // Проверяем семейство/подтип, если нужно
        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family) continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }
        Count++;
    }
    return Count;
}

float UChoreManagerSubsystem::GetBestPerformance(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, const FString& MetricName) const
{
    float Best = 0.0f;
    bool bFirst = true;
    for (const FChoreHistoryEntry& Entry : History)
    {
        if (!ChoreId.IsNone() && Entry.ChoreId != ChoreId) continue;
        if (bUseFamily || bUseSubtype)
        {
            UChoreDefinition* Def = GetChoreDefinition(Entry.ChoreId);
            if (!Def) continue;
            if (bUseFamily && Def->Family != Family) continue;
            if (bUseSubtype && Def->Subtype != Subtype) continue;
        }

        float Value = 0.0f;
        if (MetricName == TEXT("CompletionTimeSeconds")) Value = Entry.Performance.CompletionTimeSeconds;
        else if (MetricName == TEXT("Mistakes")) Value = Entry.Performance.Mistakes;
        else if (MetricName == TEXT("Accuracy")) Value = Entry.Performance.Accuracy;
        else if (MetricName == TEXT("Quantity")) Value = Entry.Performance.Quantity;
        else continue;

        if (bFirst || Value > Best) { Best = Value; bFirst = false; }
    }
    return Best;
}

bool UChoreManagerSubsystem::GetLastResult(FName ChoreId) const
{
    for (int32 i = History.Num() - 1; i >= 0; --i)
    {
        if (History[i].ChoreId == ChoreId)
            return History[i].bSucceeded;
    }
    return false;
}

// ---- Обработчики событий ----
void UChoreManagerSubsystem::HandleEvent(const FOutcomeEventBase& Outcome)
{
    // Перебираем все задания и проверяем, не выполнились ли их условия доступности
    // Это может быть дорого, но для небольшого количества заданий допустимо.
    // Альтернатива – хранить маппинг событий на задания.
    for (auto& Pair : ActiveStates)
    {
        FName ChoreId = Pair.Key;
        FChoreState& State = Pair.Value;
        if (State.Status != EChoreStatus::Unavailable && State.Status != EChoreStatus::RetryAvailable)
            continue;

        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (!Def || !Def->AvailabilityCondition) continue;

        // Проверяем, скомпилировано ли условие
        if (!Def->AvailabilityCondition->GetCondition().IsValid())
        {
            Def->AvailabilityCondition->CompileCondition();
        }
        if (Def->AvailabilityCondition->GetCondition().IsValid() &&
            Def->AvailabilityCondition->GetCondition()->Evaluate(Outcome))
        {
            OfferChore(ChoreId);
        }
    }
}

void UChoreManagerSubsystem::HandleMissionRequest(const FOutcomeEventBase& Outcome)
{
    UChoreMissionRequestPayload* Req = Cast<UChoreMissionRequestPayload>(Outcome.Payload);
    if (!Req) return;

    FName ChoreId = Req->ChoreId;
    if (!Definitions.Contains(ChoreId))
    {
        UE_LOG(LogTemp, Warning, TEXT("ChoreManager: Mission request for unknown ChoreId '%s'"), *ChoreId.ToString());
        return;
    }

    // Проверяем, доступна ли хора (можно запускать даже если она не доступна? – по дизайну миссия может запускать любую)
    // Для миссий создаём временное состояние, если его нет
    if (!ActiveStates.Contains(ChoreId))
    {
        FChoreState NewState;
        NewState.ChoreId = ChoreId;
        NewState.Status = EChoreStatus::Active;
        ActiveStates.Add(ChoreId, NewState);
    }
    else
    {
        FChoreState& State = ActiveStates[ChoreId];
        // Если хора уже выполняется, игнорируем
        if (State.Status == EChoreStatus::Active || State.Status == EChoreStatus::Succeeded)
            return;
        // Иначе переводим в активное
        State.Status = EChoreStatus::Active;
    }

    // Публикуем событие о старте
    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (EventBus)
    {
        FOutcomeEventBase Event;
        Event.OutcomeType = EOutcomeType::Chore;
        Event.OutcomeChore = EOutcomeChore::ChoreStarted;
        Event.Payload = Req; // переиспользуем payload
        EventBus->PublishOutcome(Event);
    }
}

void UChoreManagerSubsystem::HandleChoreCompletion(const FOutcomeEventBase& Outcome)
{
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
        AddHistoryEntry(ChoreId, true, Result->Performance);
        // Обработка повторяемости
        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def && Def->bIsRepeatable)
        {
            if (Def->RetryBehavior == EChoreRetryBehavior::Immediate)
                UpdateChoreState(ChoreId, EChoreStatus::RetryAvailable);
            else if (Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
                RegisterAvailabilityHandler(Def);
        }
    }
    else
    {
        UpdateChoreState(ChoreId, EChoreStatus::Failed);
        AddHistoryEntry(ChoreId, false, Result->Performance);
        // Обработка повтора
        UChoreDefinition* Def = GetChoreDefinition(ChoreId);
        if (Def && Def->RetryBehavior == EChoreRetryBehavior::Immediate)
            UpdateChoreState(ChoreId, EChoreStatus::RetryAvailable);
        else if (Def && Def->RetryBehavior == EChoreRetryBehavior::Conditional && Def->ReactivationCondition)
            RegisterAvailabilityHandler(Def);
    }

    // Если это результат миссионной хоры (передаём дальше)
    if (!Result->MissionId.IsNone())
    {
        // Перепубликуем для миссии
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
/*
void UChoreManagerSubsystem::HandleCheckRequest(const FOutcomeEventBase& Outcome)
{
    // Если кто-то запрашивает состояние хоры (по аналогии с MissionCheckCondition)
    UCheckRequestPayload* Req = Cast<UCheckRequestPayload>(Outcome.Payload);
    if (!Req) return;

    // Предположим, что запрос содержит ChoreId в строке
    FString ChoreIdStr = Req->PropertyToCheck == ECheckProperty::Name ? Req->StringValue : FString();
    if (ChoreIdStr.IsEmpty()) return;
    FName ChoreId = FName(*ChoreIdStr);

    bool bApproved = false;
    FString Reason;

    if (ActiveStates.Contains(ChoreId))
    {
        bApproved = true;
    }
    else
    {
        Reason = TEXT("Chore not found");
    }

    UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus) return;

    UCheckResponsePayload* Resp = EventBus->CreatePayload<UCheckResponsePayload>();
    Resp->TransactionId = Req->TransactionId;
    Resp->bApproved = bApproved;
    Resp->Reason = Reason;

    FOutcomeEventBase Reply;
    Reply.OutcomeType = EOutcomeType::Chore;
    Reply.OutcomeChore = EOutcomeChore::CheckResponse;
    Reply.Payload = Resp;
    EventBus->PublishOutcome(Reply);
}
*/
// ---- Сохранение / Загрузка ----
void UChoreManagerSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // Сохраняем состояния (кроме Unavailable)
    TArray<TSharedPtr<FJsonValue>> StateArray;
    for (const auto& Pair : ActiveStates)
    {
        const FChoreState& State = Pair.Value;
        if (State.Status == EChoreStatus::Unavailable) continue;
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("ChoreId"), State.ChoreId.ToString());
        Obj->SetNumberField(TEXT("Status"), (uint8)State.Status);
        Obj->SetStringField(TEXT("AcceptTime"), State.AcceptTime.ToIso8601());
        Obj->SetStringField(TEXT("Deadline"), State.Deadline.ToIso8601());
        Obj->SetNumberField(TEXT("AttemptCount"), State.AttemptCount);
        Obj->SetBoolField(TEXT("bSucceeded"), State.bSucceeded);
        Obj->SetBoolField(TEXT("bRewardIssued"), State.bRewardIssued);
        // Сохраняем производительность
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
        Obj->SetBoolField(TEXT("bSucceeded"), Entry.bSucceeded);
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

    // Очищаем текущее состояние (кроме определений, которые уже загружены)
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
            Obj->TryGetStringField(TEXT("Deadline"), DeadlineStr);
            FDateTime::ParseIso8601(*DeadlineStr, State.Deadline);
            Obj->TryGetNumberField(TEXT("AttemptCount"), State.AttemptCount);
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

            // Перезапускаем таймер, если нужно
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
            Obj->TryGetBoolField(TEXT("bSucceeded"), Entry.bSucceeded);
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

    bLoadComplete = true;
}

// ---- Вспомогательные методы ----
void UChoreManagerSubsystem::EvaluateAllAvailability()
{
    // Используется после загрузки или регистрации новых определений
    // Можно пройтись по всем и проверить условия вручную, публикуя OfferChore
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
                    // Создаём пустое событие (не идеально, но для простоты)
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