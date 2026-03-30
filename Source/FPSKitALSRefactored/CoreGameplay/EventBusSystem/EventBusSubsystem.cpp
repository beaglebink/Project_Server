#include "EventBusSubsystem.h"
#include "OutcomeConditionAsset.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
	for (const FOutcomeHandlerEntry& Entry : Handlers)
	{
		if (!Entry.Query.IsValid() || Entry.Query->Evaluate(Outcome))
		{
			Entry.Handler.ExecuteIfBound(Outcome);
		}
	}
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

	const uint32 NewId = NextHandleId++;
	Handlers.Add(FOutcomeHandlerEntry(NewId, MoveTemp(Handler), ConditionAsset->GetCondition()));

	UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Registered handler [%u] - condition: %s"),
		NewId, *ConditionAsset->GetConditionDescription());

	return FOutcomeHandlerHandle(NewId);
}

void UEventBusSubsystem::UnregisterHandler(FOutcomeHandlerHandle& Handle)
{
	if (!Handle.IsValid())
	{
		return;
	}

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
