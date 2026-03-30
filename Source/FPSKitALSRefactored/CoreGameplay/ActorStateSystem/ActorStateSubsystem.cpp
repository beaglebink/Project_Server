#include "ActorStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	if (DialogueStartedCondition)
	{
		EventBus->RegisterHandler(
			DialogueStartedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleDialogueStarted)
		);
	}

	if (MissionOrGhostCondition)
	{
		EventBus->RegisterHandler(
			MissionOrGhostCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleMissionOrGhost)
		);
	}
}

void UActorStateSubsystem::HandleDialogueStarted(const FOutcomeEventBase& Outcome)
{
	const FActorStateOutcome& State = static_cast<const FActorStateOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Dialogue started - %s"), *State.StateChangeType);

	// Typed broadcast - Blueprint gets FActorStateOutcome directly
	// (Типизированная рассылка - Blueprint получает FActorStateOutcome напрямую)
	OnDialogueStarted.Broadcast(State);
}

void UActorStateSubsystem::HandleMissionOrGhost(const FOutcomeEventBase& Outcome)
{
	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Mission or Ghost - SpawnGroup: %d"),
		static_cast<int32>(Outcome.OutcomeSpawnGroup));

	// Base type - event can be either MissionCompleted or GhostCleared
	// Blueprint can check OutcomeType to determine which one
	// (Базовый тип - событие может быть MissionCompleted или GhostCleared)
	// (Blueprint может проверить OutcomeType чтобы определить какое именно)
	OnMissionOrGhostWithSpawn.Broadcast(Outcome);
}

