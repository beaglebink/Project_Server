#include "ActorStateSubsystem.h"
#include "ActorStateOutcome.h"
#include "EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeQuery.h"
#include "../WorldStateSystem/MissionCompletedOutcome.h"
#include "../SpawnGroupSystem/MissionCompleteGhostOutcome.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Handler 1: DialogueStarted (any) (Обработчик 1: DialogueStarted (любой))
		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::OnDialogueStarted),
			FOutcomeQueryBuilder::Type(EOutcomeType::DialogueStarted)
		);

		// Handler 2: (MissionCompleted OR GhostCleared) AND SpawnGroup != Default (Обработчик 2: (MissionCompleted ИЛИ GhostCleared) И не Default спауна)
		auto MissionOrGhost = FOutcomeQueryBuilder::Or();
		MissionOrGhost->Add(FOutcomeQueryBuilder::Type(EOutcomeType::MissionCompleted));
		MissionOrGhost->Add(FOutcomeQueryBuilder::Type(EOutcomeType::GhostCleared));
		
		auto ComplexQuery = FOutcomeQueryBuilder::And();
		ComplexQuery->Add(MissionOrGhost);
		ComplexQuery->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::SpawnGroup(EOutcomeSpawnGroup::Default)));

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::OnMissionOrGhostWithSpawn),
			ComplexQuery
		);

		UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: Registered 2 event handlers"));
	}
}

void UActorStateSubsystem::OnDialogueStarted(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FActorStateOutcome* DialogueOutcome = static_cast<const FActorStateOutcome*>(&Outcome);
	
	if (DialogueOutcome)
	{
		// Handle dialogue-specific data (Обработка специфичных данных диалога)
		UE_LOG(LogTemp, Log, 
			TEXT("ActorStateSubsystem: Dialogue started - StateType: %s, Context: %s"),
			*DialogueOutcome->StateChangeType,
			*DialogueOutcome->ContextId
		);
	}
}

void UActorStateSubsystem::OnMissionOrGhostWithSpawn(const FOutcomeEventBase& Outcome)
{
	// Single handler for both types of events (Один обработчик для обоих типов событий)
	// System has already filtered events by condition: (MissionCompleted OR GhostCleared) AND SpawnGroup != Default (Система уже отфильтровала события по условию: (MissionCompleted ИЛИ GhostCleared) И не Default спауна)
	
	// Cast to specialized type for this event (Кастируем в специализированный тип для этого события)
	const FMissionCompleteGhostOutcome* MissionOrGhostWithSpawn = static_cast<const FMissionCompleteGhostOutcome*>(&Outcome);

	if (MissionOrGhostWithSpawn)
	{
		// Handle event-specific data (Обработка специфичных данных для этого события)
		UE_LOG(LogTemp, Log,
			TEXT("ActorStateSubsystem: Event with non-Default spawn")
			TEXT(" | SpawnGroup: %d, Actor: %d, GhostType: %s, SomeData1: %s, SomeData2: %f"),
			static_cast<int32>(MissionOrGhostWithSpawn->OutcomeSpawnGroup),
			static_cast<int32>(MissionOrGhostWithSpawn->OutcomeActor),
			*MissionOrGhostWithSpawn->GhostType,
			*MissionOrGhostWithSpawn->SomeData1,
			MissionOrGhostWithSpawn->SomeData2
		);
	}
}
