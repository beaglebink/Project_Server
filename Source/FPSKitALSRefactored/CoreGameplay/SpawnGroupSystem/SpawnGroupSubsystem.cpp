#include "SpawnGroupSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void USpawnGroupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus || !GhostClearedCondition) return;

	EventBus->RegisterHandler(
		GhostClearedCondition,
		FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleGhostCleared)
	);
}

void USpawnGroupSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	const FGhostClearedOutcome& Ghost = static_cast<const FGhostClearedOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Ghost %s cleared - SpawnGroup: %d"),
		*Ghost.GhostType,
		static_cast<int32>(Ghost.OutcomeSpawnGroup));

	OnGhostCleared.Broadcast(Ghost);
}
