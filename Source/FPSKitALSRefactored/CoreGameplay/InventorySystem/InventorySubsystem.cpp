#include "InventorySubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus || !ItemAcquiredCondition) return;

	EventBus->RegisterHandler(
		ItemAcquiredCondition,
		FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleItemAcquired)
	);
}

void UInventorySubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	const FItemAcquiredOutcome& Item = static_cast<const FItemAcquiredOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired - Object: %s"),
		*Item.ObjectId.ToString());

	OnItemAcquired.Broadcast(Item);
}
