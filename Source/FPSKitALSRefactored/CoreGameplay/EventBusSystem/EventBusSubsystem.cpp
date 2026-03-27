#include "EventBusSubsystem.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
	OnOutcomeEvent.Broadcast(Outcome);
}
