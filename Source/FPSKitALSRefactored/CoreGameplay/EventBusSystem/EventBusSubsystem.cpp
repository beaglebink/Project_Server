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

void UEventBusSubsystem::RegisterHandler(UOutcomeConditionAsset* ConditionAsset, FOutcomeHandlerDelegate Handler)
{
	if (!ConditionAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: RegisterHandler - ConditionAsset is null"));
		return;
	}

	if (!Handler.IsBound())
	{
		UE_LOG(LogTemp, Warning, TEXT("EventBusSubsystem: RegisterHandler - Handler is not bound"));
		return;
	}

	ConditionAsset->CompileCondition();
	Handlers.Add(FOutcomeHandlerEntry(MoveTemp(Handler), ConditionAsset->GetCondition()));

	UE_LOG(LogTemp, Log, TEXT("EventBusSubsystem: Registered handler - condition: %s"),
		*ConditionAsset->GetConditionDescription());
}
