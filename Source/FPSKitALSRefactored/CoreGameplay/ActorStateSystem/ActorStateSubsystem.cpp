#include "ActorStateSubsystem.h"
#include "EventBusSubsystem.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &UActorStateSubsystem::HandleOutcome);
    }
}

void UActorStateSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == "DialogueStarted")
    {
        UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Dialogue started, updating NPC state."));
    }
}
