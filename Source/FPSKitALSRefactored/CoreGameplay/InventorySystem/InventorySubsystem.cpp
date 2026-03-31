#include "InventorySubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ItemAcquiredPayload.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (ItemAcquiredCondition)
	{
		SubscribeItemAcquired();
	}
}

void UInventorySubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UInventorySubsystem::SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition)
{
	if (ItemAcquiredCondition == NewCondition) return;
	ItemAcquiredCondition = NewCondition;
	if (ItemAcquiredHandle.IsValid())
	{
		UnsubscribeItemAcquired();
	}
	SubscribeItemAcquired();
}

void UInventorySubsystem::SubscribeItemAcquired()
{
	if (ItemAcquiredHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem: Already subscribed to ItemAcquired"));
		return;
	}

	if (!ItemAcquiredCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem: ItemAcquiredCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		ItemAcquiredHandle = EventBus->RegisterHandler(
			ItemAcquiredCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleItemAcquired)
		);

		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to ItemAcquired (handle=%u)"), ItemAcquiredHandle.GetId());
	}
}

void UInventorySubsystem::UnsubscribeItemAcquired()
{
	if (!ItemAcquiredHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(ItemAcquiredHandle);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Unsubscribed ItemAcquired (handle=%u)"), ItemAcquiredHandle.GetId());
	}
	ItemAcquiredHandle.Invalidate();
}

void UInventorySubsystem::UnsubscribeAll()
{
	UnsubscribeItemAcquired();
}

void UInventorySubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	if (ItemAcquiredCondition)
	{
		auto Query = ItemAcquiredCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("InventorySubsystem: Incoming outcome does not satisfy ItemAcquiredCondition -> ignoring"));
			return;
		}
	}

	if (UItemAcquiredPayload* P = Cast<UItemAcquiredPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired - Object: %s"),
			*P->ObjectId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("InventorySubsystem: HandleItemAcquired called with no payload or unexpected payload type"));
	}

	OnItemAcquired.Broadcast(Outcome);
}
