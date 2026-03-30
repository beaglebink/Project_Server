#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	SubscribeGhostCleared();
	SubscribeItemAcquired();
}

void UInteriorSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UInteriorSubsystem::SubscribeGhostCleared()
{
	if (GhostClearedHandle.IsValid() || !GhostClearedCondition) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleGhostCleared)
		);
	}
}

void UInteriorSubsystem::SubscribeItemAcquired()
{
	if (ItemAcquiredHandle.IsValid() || !ItemAcquiredCondition) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		ItemAcquiredHandle = EventBus->RegisterHandler(
			ItemAcquiredCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleItemAcquired)
		);
	}
}

void UInteriorSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
	}
}

void UInteriorSubsystem::UnsubscribeItemAcquired()
{
	if (!ItemAcquiredHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(ItemAcquiredHandle);
	}
}

void UInteriorSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeItemAcquired();
}

void UInteriorSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	const FGhostClearedOutcome& Ghost = static_cast<const FGhostClearedOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Ghost cleared - Interior: %d"),
		static_cast<int32>(Ghost.OutcomeInterior));

	OnGhostCleared.Broadcast(Ghost);
}

void UInteriorSubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	const FItemAcquiredOutcome& Item = static_cast<const FItemAcquiredOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Item acquired - Object: %s"),
		*Item.ObjectId.ToString());

	OnItemAcquired.Broadcast(Item);
}

