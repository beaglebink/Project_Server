#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus || !MissionCompletedCondition) return;

	EventBus->RegisterHandler(
		MissionCompletedCondition,
		FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleMissionCompleted)
	);
}

void UWorldStateSubsystem::HandleMissionCompleted(const FOutcomeEventBase& Outcome)
{
	const FMissionCompletedOutcome& Mission = static_cast<const FMissionCompletedOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Mission completed - FactKey: %s"),
		*Mission.GlobalFactKey);

	OnMissionCompleted.Broadcast(Mission);
}


