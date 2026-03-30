#include "SpawnGroupSubsystem.h"
#include "GhostClearedOutcome.h"
#include "EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeQuery.h"

void USpawnGroupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Handler: GhostCleared AND SpawnGroup != Default AND (Mission != Default OR Actor != Default) (Обработчик: GhostCleared И спаун-группа не Default И (миссия не Default ИЛИ актёр не Default))
		auto SpawnQuery = FOutcomeQueryBuilder::And();
		SpawnQuery->Add(FOutcomeQueryBuilder::Type(EOutcomeType::GhostCleared));
		SpawnQuery->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::SpawnGroup(EOutcomeSpawnGroup::Default)));
		
		auto MissionOrActor = FOutcomeQueryBuilder::Or();
		MissionOrActor->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Mission(EOutcomeMission::Default)));
		MissionOrActor->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Actor(EOutcomeActor::Default)));
		SpawnQuery->Add(MissionOrActor);

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::OnGhostClearedWithSpawn),
			SpawnQuery
		);

		UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSubsystem: Registered event handler"));
	}
}

void USpawnGroupSubsystem::OnGhostClearedWithSpawn(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FGhostClearedOutcome* GhostCleared = static_cast<const FGhostClearedOutcome*>(&Outcome);
	
	if (GhostCleared)
	{
		// Handle ghost-specific data (Обработка специфичных данных привидения)
		UE_LOG(LogTemp, Log,
			TEXT("SpawnGroupSubsystem: Ghost %s cleared - resetting spawn group")
			TEXT(" | SpawnGroup: %d, Mission: %d, Actor: %d"),
			*GhostCleared->GhostType,
			static_cast<int32>(GhostCleared->OutcomeSpawnGroup),
			static_cast<int32>(GhostCleared->OutcomeMission),
			static_cast<int32>(GhostCleared->OutcomeActor)
		);
	}
}
