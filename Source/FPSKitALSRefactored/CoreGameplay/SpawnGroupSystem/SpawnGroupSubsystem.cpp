#include "SpawnGroupSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void USpawnGroupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	SubscribeGhostCleared();
}

void USpawnGroupSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void USpawnGroupSubsystem::SubscribeGhostCleared()
{
	if (GhostClearedHandle.IsValid() || !GhostClearedCondition) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleGhostCleared)
		);
	}
}

void USpawnGroupSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
	}
}

void USpawnGroupSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
}

void USpawnGroupSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	const FGhostClearedOutcome& Ghost = static_cast<const FGhostClearedOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Ghost %s cleared"), *Ghost.GhostType);

	OnGhostCleared.Broadcast(Ghost);
}
