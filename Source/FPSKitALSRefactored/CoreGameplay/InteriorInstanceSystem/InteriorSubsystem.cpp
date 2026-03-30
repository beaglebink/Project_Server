#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	if (GhostClearedCondition)
	{
		EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleGhostCleared)
		);
	}

	if (ItemAcquiredCondition)
	{
		EventBus->RegisterHandler(
			ItemAcquiredCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleItemAcquired)
		);
	}
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

