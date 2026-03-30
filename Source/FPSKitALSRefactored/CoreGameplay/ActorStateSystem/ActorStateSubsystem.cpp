#include "ActorStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SubscribeDialogueStarted();
	SubscribeMissionOrGhost();
}

void UActorStateSubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UActorStateSubsystem::SubscribeDialogueStarted()
{
	if (DialogueStartedHandle.IsValid() || !DialogueStartedCondition) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		DialogueStartedHandle = EventBus->RegisterHandler(
			DialogueStartedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleDialogueStarted)
		);
	}
}

void UActorStateSubsystem::SubscribeMissionOrGhost()
{
	if (MissionOrGhostHandle.IsValid() || !MissionOrGhostCondition) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		MissionOrGhostHandle = EventBus->RegisterHandler(
			MissionOrGhostCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleMissionOrGhost)
		);
	}
}

void UActorStateSubsystem::UnsubscribeDialogueStarted()
{
	if (!DialogueStartedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(DialogueStartedHandle);
	}
}

void UActorStateSubsystem::UnsubscribeMissionOrGhost()
{
	if (!MissionOrGhostHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(MissionOrGhostHandle);
	}
}

void UActorStateSubsystem::UnsubscribeAll()
{
	UnsubscribeDialogueStarted();
	UnsubscribeMissionOrGhost();
}

void UActorStateSubsystem::HandleDialogueStarted(const FOutcomeEventBase& Outcome)
{
	const FActorStateOutcome& State = static_cast<const FActorStateOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Dialogue started - %s"), *State.StateChangeType);

	OnDialogueStarted.Broadcast(State);
}

void UActorStateSubsystem::HandleMissionOrGhost(const FOutcomeEventBase& Outcome)
{
	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Mission or Ghost - SpawnGroup: %d"),
		static_cast<int32>(Outcome.OutcomeSpawnGroup));

	OnMissionOrGhostWithSpawn.Broadcast(Outcome);
}

