#include "ActorStateSubsystem.h"
#include "ActorStateOutcome.h"
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
    switch (Outcome.OutcomeType)
    {
        case EOutcomeType::DialogueStarted:
        {
            const FActorStateOutcome* ActorStateOutcome = static_cast<const FActorStateOutcome*>(&Outcome);
            if (ActorStateOutcome)
            {
                UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Dialogue started, updating NPC state. StateChangeType: %s"), *ActorStateOutcome->StateChangeType);
            }
            break;
        }
        default:
            break;
    }
}
