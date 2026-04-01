#include "EventBusSubsystem.h"
#include "OutcomeConditionAsset.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
	for (const FOutcomeHandlerEntry& Entry : Handlers)
	{
		// Debug: log condition description and evaluation result at Verbose level
		if (Entry.Query.IsValid())
		{
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
		else
		{
			// If Query is invalid we consider it as "match everything" historically.
			// Log Warning so we can find unexpected registrations.
			UE_LOG(LogTemp, Warning, TEXT("EventBus: Handler[%u] has invalid Query -> invoking handler (consider preventing registration with invalid condition)"),
				Entry.HandleId);
			Entry.Handler.ExecuteIfBound(Outcome);
		}
	}
}

UOutcomePayload* UEventBusSubsystem::CreatePayload(TSubclassOf<UOutcomePayload> PayloadClass)
{
	if (!PayloadClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem::CreatePayload - PayloadClass is null"));
		return nullptr;
	}
	// GameInstance as Outer - object lives for the session, no GC risk during event dispatch
	// (GameInstance как Outer - объект живёт всю сессию, нет риска GC при доставке события)
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

	// Compile condition and check that it produced a valid Query.
	ConditionAsset->CompileCondition();
	TSharedPtr<IOutcomeCondition> Compiled = ConditionAsset->GetCondition();
	if (!Compiled.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: RegisterHandler - ConditionAsset [%s] compiled to invalid Query -> registration rejected"),
			*ConditionAsset->GetName());
		return FOutcomeHandlerHandle();
	}

	const uint32 NewId = NextHandleId++;
	Handlers.Add(FOutcomeHandlerEntry(NewId, MoveTemp(Handler), Compiled));

	UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Registered handler [%u] - condition: %s"),
		NewId, *ConditionAsset->GetConditionDescription());

	return FOutcomeHandlerHandle(NewId);
}

void UEventBusSubsystem::UnregisterHandler(FOutcomeHandlerHandle& Handle)
{
	if (!Handle.IsValid()) return;

	const uint32 IdToRemove = Handle.GetId();
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
