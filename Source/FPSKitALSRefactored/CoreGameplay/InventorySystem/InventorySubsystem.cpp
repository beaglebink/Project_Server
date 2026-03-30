#include "InventorySubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SubscribeItemAcquired();
}

void UInventorySubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UInventorySubsystem::SubscribeItemAcquired()
{
	if (ItemAcquiredHandle.IsValid() || !ItemAcquiredCondition) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		ItemAcquiredHandle = EventBus->RegisterHandler(
			ItemAcquiredCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleItemAcquired)
		);
	}
}

void UInventorySubsystem::UnsubscribeItemAcquired()
{
	if (!ItemAcquiredHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(ItemAcquiredHandle);
	}
}

void UInventorySubsystem::UnsubscribeAll()
{
	UnsubscribeItemAcquired();
}

void UInventorySubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	const FItemAcquiredOutcome& Item = static_cast<const FItemAcquiredOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired - Object: %s"),
		*Item.ObjectId.ToString());

	// One-shot pattern example: unsubscribe after first event
	// (Пример одноразового обработчика: отписка после первого события)
	// UnsubscribeItemAcquired();

	OnItemAcquired.Broadcast(Item);
}
