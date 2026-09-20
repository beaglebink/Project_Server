#include "EventBusSubsystem.h"
#include "OutcomeConditionAsset.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
    // FIX: если мы уже в teardown — вообще не принимаем реентерантные публикации,
    // чтобы не накапливать в PendingEvents события, которые будут диспатчиться
    // по мёртвым BP-объектам (это и был путь крэша в стеке).
    if (UWorld* W = GetWorld(); W && W->bIsTearingDown)
    {
        return;
    }

    // Защита от рекурсивного входа
    if (bIsPublishing)
    {
        FScopeLock Lock(&HandlersCriticalSection);
        PendingEvents.Add(Outcome);
        UE_LOG(LogTemp, Verbose, TEXT("EventBusSubsystem: Reentrant PublishOutcome queued."));
        return;
    }

    // FIX: снимаем снимок хэндлеров под локом, а сами делегаты исполняем ВНЕ лока.
    // Иначе BP-код держит CRITICAL_SECTION и может реентерабельно трогать реестр.
    TArray<FOutcomeHandlerEntry> LocalHandlers;
    {
        FScopeLock Lock(&HandlersCriticalSection);
        bIsPublishing = true;
        bDispatching = true;
        LocalHandlers = Handlers; // копия — на случай реентерантных модификаций
    }

    for (int32 i = 0; i < LocalHandlers.Num(); ++i)
    {
        FOutcomeHandlerEntry& Entry = LocalHandlers[i];
        if (Entry.bPendingRemove) continue;
        if (!Entry.Query.IsValid()) continue;

        const bool bResult = Entry.Query->Evaluate(Outcome);
        if (!bResult) continue;

        if (Entry.Handler.IsBound())
        {
            Entry.Handler.Execute(Outcome);
        }
        else if (Entry.BlueprintDelegate.IsBound())
        {
            // FIX: проверяем, что BP-объект ещё валиден и не в процессе разрушения.
            UObject* BPObj = Entry.BlueprintDelegate.GetUObject();
            if (IsValid(BPObj) &&
                !BPObj->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
            {
                Entry.BlueprintDelegate.Execute(Outcome);
            }
            else
            {
                // Хэндлер на разрушаемом/мёртвом объекте — помечаем на удаление.
                FScopeLock Lock(&HandlersCriticalSection);
                for (FOutcomeHandlerEntry& E : Handlers)
                {
                    if (E.HandleId == Entry.HandleId)
                    {
                        E.bPendingRemove = true;
                        break;
                    }
                }
            }
        }
    }

    // Применяем отложенные операции регистрации/отписки и чистим помеченные на удаление.
    {
        FScopeLock Lock(&HandlersCriticalSection);
        bDispatching = false;

        for (const FPendingOperation& Op : PendingOperations)
        {
            if (Op.Type == FPendingOperation::EType::Unregister)
            {
                for (FOutcomeHandlerEntry& E : Handlers)
                {
                    if (E.HandleId == Op.HandleId)
                    {
                        E.bPendingRemove = true;
                        break;
                    }
                }
            }
            else if (Op.Type == FPendingOperation::EType::Register)
            {
                Handlers.Add(Op.Entry);
            }
        }
        PendingOperations.Empty();

        CleanupPendingRemoves();

        bIsPublishing = false;
    }

    // Теперь обрабатываем отложенные события (если они есть)
    ProcessPendingEvents();
}

void UEventBusSubsystem::ProcessPendingEvents()
{
    // Защита от рекурсивной обработки очереди
    if (bIsProcessingPending || PendingEvents.Num() == 0)
        return;

    bIsProcessingPending = true;

    // Копируем очередь, чтобы избежать конфликтов при добавлении новых событий
    TArray<FOutcomeEventBase> EventsToProcess = MoveTemp(PendingEvents);
    // Очередь теперь пуста, но во время обработки могут добавиться новые

    for (const FOutcomeEventBase& Ev : EventsToProcess)
    {
        // Рекурсивный вызов PublishOutcome для отложенных событий
        // Поскольку bIsPublishing сейчас false, они будут опубликованы синхронно
        PublishOutcome(Ev);
    }

    // После обработки скопированных событий проверяем, не добавились ли новые
    // во время обработки (рекурсивно). Если добавились – обрабатываем их рекурсивно.
    if (PendingEvents.Num() > 0)
    {
        // Рекурсивный вызов для обработки новых отложенных событий
        ProcessPendingEvents();
    }

    bIsProcessingPending = false;
}

void UEventBusSubsystem::CleanupPendingRemoves()
{
    // Предполагается, что вызывается уже с захваченным мьютексом
    Handlers.RemoveAll([](const FOutcomeHandlerEntry& Entry)
        {
            return Entry.bPendingRemove;
        });
}

UOutcomePayload* UEventBusSubsystem::CreatePayload(TSubclassOf<UOutcomePayload> PayloadClass)
{
    if (!PayloadClass) return nullptr;
    return NewObject<UOutcomePayload>(GetGameInstance(), PayloadClass);
}

FOutcomeHandlerHandle UEventBusSubsystem::RegisterHandler(
    UOutcomeConditionAsset* ConditionAsset,
    FOutcomeHandlerDelegate Handler)
{
    FScopeLock Lock(&HandlersCriticalSection);

    if (!ConditionAsset || !Handler.IsBound())
        return FOutcomeHandlerHandle();

    ConditionAsset->CompileCondition();
    TSharedPtr<IOutcomeCondition> Compiled = ConditionAsset->GetCondition();
    if (!Compiled.IsValid())
        return FOutcomeHandlerHandle();

    // Проверка на дублирование для C++ обработчиков
    bool bDuplicate = false;
    for (const FOutcomeHandlerEntry& Existing : Handlers)
    {
        if (Existing.ConditionAsset == ConditionAsset &&
            Existing.Handler.IsBound() &&
            Existing.Handler.GetUObject() == Handler.GetUObject())
        {
            bDuplicate = true;
            UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: Duplicate C++ handler detected for ConditionAsset %s, skipping registration."),
                *ConditionAsset->GetName());
            break;
        }
    }

    if (bDuplicate)
        return FOutcomeHandlerHandle();

    const uint32 NewId = NextHandleId++;
    FOutcomeHandlerEntry NewEntry(NewId, MoveTemp(Handler), Compiled, ConditionAsset);

    if (bDispatching)
    {
        FPendingOperation Op;
        Op.Type = FPendingOperation::EType::Register;
        Op.Entry = MoveTemp(NewEntry);
        PendingOperations.Add(MoveTemp(Op));
    }
    else
    {
        Handlers.Add(MoveTemp(NewEntry));
    }
    return FOutcomeHandlerHandle(NewId);
}

FOutcomeHandlerHandle UEventBusSubsystem::RegisterBlueprintHandler(
    UOutcomeConditionAsset* ConditionAsset,
    FOnOutcomeEvent Delegate)
{
    FScopeLock Lock(&HandlersCriticalSection);

    if (!ConditionAsset || !Delegate.IsBound())
        return FOutcomeHandlerHandle();

    ConditionAsset->CompileCondition();
    TSharedPtr<IOutcomeCondition> Compiled = ConditionAsset->GetCondition();
    if (!Compiled.IsValid())
        return FOutcomeHandlerHandle();

    // Проверка на дублирование для Blueprint обработчиков
    bool bDuplicate = false;
    for (const FOutcomeHandlerEntry& Existing : Handlers)
    {
        if (Existing.ConditionAsset == ConditionAsset &&
            Existing.BlueprintDelegate.IsBound() &&
            Existing.BlueprintDelegate.GetUObject() == Delegate.GetUObject() &&
            Existing.BlueprintDelegate.GetFunctionName() == Delegate.GetFunctionName())
        {
            bDuplicate = true;
            UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: Duplicate Blueprint handler detected for ConditionAsset %s, skipping registration."),
                *ConditionAsset->GetName());
            break;
        }
    }

    if (bDuplicate)
        return FOutcomeHandlerHandle();

    const uint32 NewId = NextHandleId++;
    FOutcomeHandlerEntry NewEntry(NewId, Delegate, Compiled, ConditionAsset);

    if (bDispatching)
    {
        FPendingOperation Op;
        Op.Type = FPendingOperation::EType::Register;
        Op.Entry = MoveTemp(NewEntry);
        PendingOperations.Add(MoveTemp(Op));
    }
    else
    {
        Handlers.Add(MoveTemp(NewEntry));
    }
    return FOutcomeHandlerHandle(NewId);
}

void UEventBusSubsystem::UnregisterHandler(FOutcomeHandlerHandle& Handle)
{
    FScopeLock Lock(&HandlersCriticalSection);

    if (!Handle.IsValid())
        return;

    const uint32 IdToRemove = Handle.GetId();

    if (bDispatching)
    {
        FPendingOperation Op;
        Op.Type = FPendingOperation::EType::Unregister;
        Op.HandleId = IdToRemove;
        PendingOperations.Add(Op);
        Handle.Invalidate();
        return;
    }

    for (FOutcomeHandlerEntry& Entry : Handlers)
    {
        if (Entry.HandleId == IdToRemove)
        {
            Entry.bPendingRemove = true;
            break;
        }
    }
    Handle.Invalidate();
    CleanupPendingRemoves();
}

void UEventBusSubsystem::BeginDestroy()
{
    Super::BeginDestroy();
    Handlers.Empty();
    PendingEvents.Empty();
}