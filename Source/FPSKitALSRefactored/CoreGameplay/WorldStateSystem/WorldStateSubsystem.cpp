#include "WorldStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SubscribeMissionCompleted();
}

void UWorldStateSubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UWorldStateSubsystem::SubscribeMissionCompleted()
{
	if (MissionCompletedHandle.IsValid() || !MissionCompletedCondition) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		MissionCompletedHandle = EventBus->RegisterHandler(
			MissionCompletedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::HandleMissionCompleted)
		);
	}
}

void UWorldStateSubsystem::UnsubscribeMissionCompleted()
{
	if (!MissionCompletedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(MissionCompletedHandle);
	}
}

void UWorldStateSubsystem::UnsubscribeAll()
{
	UnsubscribeMissionCompleted();
}

void UWorldStateSubsystem::HandleMissionCompleted(const FOutcomeEventBase& Outcome)
{
	const FMissionCompletedOutcome& Mission = static_cast<const FMissionCompletedOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("WorldStateSubsystem: Mission completed - FactKey: %s"),
		*Mission.GlobalFactKey);

	OnMissionCompleted.Broadcast(Mission);
}


