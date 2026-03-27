#include "WorldStateSubsystem.h"
#include "EventBusSubsystem.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &UWorldStateSubsystem::HandleOutcome);
    }
}

void UWorldStateSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == "MissionCompleted")
    {
        UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Mission completed, updating global facts."));
    }
}
