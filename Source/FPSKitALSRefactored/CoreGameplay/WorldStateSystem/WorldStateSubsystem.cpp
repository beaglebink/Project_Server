#include "WorldStateSubsystem.h"
#include "MissionCompletedOutcome.h"
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
		const FMissionCompletedOutcome* MissionCompletedOutcome = static_cast<const FMissionCompletedOutcome*>(&Outcome);
        if (MissionCompletedOutcome)
        {
			UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Mission completed. MissionId: %s, GlobalFactKey: %s"), *MissionCompletedOutcome->MissionId.ToString(), *MissionCompletedOutcome->GlobalFactKey);
        }
    }
}
