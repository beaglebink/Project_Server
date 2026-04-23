#/***********************************************
 * EventBus subsystem implementation
 *
 * English: Core implementation of a lightweight EventBus used to publish
 * and subscribe to Outcome events. Handlers can be registered with
 * condition assets; dispatching evaluates conditions and invokes handlers.
 *
 * Русский: Реализация подсистемы EventBus для публикации и подписки
 * на события Outcome. Обработчики регистрируются с помощью ассетов
 * условий; при рассылке выполняются условия и вызываются обработчики.
 ***********************************************/
#
#include "EventBusSubsystem.h"
#include "OutcomeConditionAsset.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
	bDispatching = true;

	for (int32 i = 0; i < Handlers.Num(); ++i)
	{
		const FOutcomeHandlerEntry& Entry = Handlers[i];

		if (!Entry.Query.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("EventBus: Handler[%u] has invalid Query -> skipping"), Entry.HandleId);
			continue;
		}

		const bool bResult = Entry.Query->Evaluate(Outcome);
		UE_LOG(LogTemp, Verbose, TEXT("EventBus: Handler[%u] Condition: %s -> Evaluate() = %s"),
			Entry.HandleId,
			*Entry.Query->Describe(),
			bResult ? TEXT("true") : TEXT("false"));

		if (bResult)
		{
			Entry.Handler.ExecuteIfBound(Outcome);
		}
	}

	bDispatching = false;

	// Выполняем отложенные операции Register/Unregister накопленные во время dispatch
	for (const FPendingOperation& Op : PendingOperations)
	{
		if (Op.Type == FPendingOperation::EType::Unregister)
		{
			Handlers.RemoveAll([&Op](const FOutcomeHandlerEntry& E)
			{
				return E.HandleId == Op.HandleId;
			});
			UE_LOG(LogTemp, Log, TEXT("EventBus: Deferred Unregistered handler [%u]"), Op.HandleId);
		}
		else if (Op.Type == FPendingOperation::EType::Register)
		{
			Handlers.Add(Op.Entry);
			UE_LOG(LogTemp, Log, TEXT("EventBus: Deferred Registered handler [%u]"), Op.Entry.HandleId);
		}
	}
	PendingOperations.Empty();
}

UOutcomePayload* UEventBusSubsystem::CreatePayload(TSubclassOf<UOutcomePayload> PayloadClass)
{
	if (!PayloadClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem::CreatePayload - PayloadClass is null"));
		return nullptr;
	}
	return NewObject<UOutcomePayload>(GetGameInstance(), PayloadClass);
}

FOutcomeHandlerHandle UEventBusSubsystem::RegisterHandler(
	UOutcomeConditionAsset* ConditionAsset,
	FOutcomeHandlerDelegate Handler)
{
	if (!ConditionAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: RegisterHandler - ConditionAsset is null"));
		return FOutcomeHandlerHandle();
	}

	if (!Handler.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: RegisterHandler - Handler is not bound"));
		return FOutcomeHandlerHandle();
	}

	ConditionAsset->CompileCondition();
	TSharedPtr<IOutcomeCondition> Compiled = ConditionAsset->GetCondition();
	if (!Compiled.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: RegisterHandler - ConditionAsset [%s] compiled to invalid Query -> registration rejected"),
			*ConditionAsset->GetName());
		return FOutcomeHandlerHandle();
	}

	const uint32 NewId = NextHandleId++;
	FOutcomeHandlerEntry NewEntry(NewId, MoveTemp(Handler), Compiled);

	if (bDispatching)
	{
		// Откладываем регистрацию до завершения текущего dispatch
		FPendingOperation Op;
		Op.Type  = FPendingOperation::EType::Register;
		Op.Entry = MoveTemp(NewEntry);
		PendingOperations.Add(MoveTemp(Op));
		UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Deferred RegisterHandler [%u] - condition: %s"),
			NewId, *ConditionAsset->GetConditionDescription());
	}
	else
	{
		Handlers.Add(MoveTemp(NewEntry));
		UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Registered handler [%u] - condition: %s"),
			NewId, *ConditionAsset->GetConditionDescription());
	}

	return FOutcomeHandlerHandle(NewId);
}

void UEventBusSubsystem::UnregisterHandler(FOutcomeHandlerHandle& Handle)
{
	if (!Handle.IsValid()) return;

	const uint32 IdToRemove = Handle.GetId();

	if (bDispatching)
	{
		// Откладываем удаление до завершения текущего dispatch
		FPendingOperation Op;
		Op.Type     = FPendingOperation::EType::Unregister;
		Op.HandleId = IdToRemove;
		PendingOperations.Add(Op);
		UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Deferred UnregisterHandler [%u]"), IdToRemove);
		Handle.Invalidate();
		return;
	}

	const int32 Removed = Handlers.RemoveAll([IdToRemove](const FOutcomeHandlerEntry& Entry)
	{
		return Entry.HandleId == IdToRemove;
	});

	if (Removed > 0)
	{
		UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Unregistered handler [%u]"), IdToRemove);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: Handler [%u] not found"), IdToRemove);
	}

	Handle.Invalidate();
}

void UEventBusSubsystem::BeginDestroy()
{
	Super::BeginDestroy();

	// Явно разрегистрируем все обработчики при уничтожении сабсистемы
	for (FOutcomeHandlerEntry& Entry : Handlers)
	{
		Entry.Handler.Unbind();
	}
	Handlers.Empty();
}
