#include "EventBusSubsystem.h"
#include "OutcomeConditionAsset.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
    FScopeLock Lock(&HandlersCriticalSection);

    bDispatching = true;

    for (int32 i = 0; i < Handlers.Num(); ++i)
    {
        const FOutcomeHandlerEntry& Entry = Handlers[i];
        if (Entry.bPendingRemove) continue;

        if (!Entry.Query.IsValid()) continue;

        const bool bResult = Entry.Query->Evaluate(Outcome);
        if (bResult)
        {
            if (Entry.Handler.IsBound())
            {
                Entry.Handler.Execute(Outcome);
            }
            else if (Entry.BlueprintDelegate.IsBound())
            {
                Entry.BlueprintDelegate.Execute(Outcome);
            }
        }
    }

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

    const uint32 NewId = NextHandleId++;
    FOutcomeHandlerEntry NewEntry(NewId, MoveTemp(Handler), Compiled);

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

    const uint32 NewId = NextHandleId++;
    FOutcomeHandlerEntry NewEntry(NewId, Delegate, Compiled);

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
    // При уничтожении подсистемы просто очищаем массив, мьютекс уже не нужен
    Handlers.Empty();
}