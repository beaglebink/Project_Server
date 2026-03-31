#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "../InventorySystem/ItemAcquiredPayload.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	if (GhostClearedCondition)
	{
		SubscribeGhostCleared();
	}
	if (ItemAcquiredCondition)
	{
		SubscribeItemAcquired();
	}
}

void UInteriorSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UInteriorSubsystem::SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition)
{
	if (GhostClearedCondition == NewCondition) return;
	GhostClearedCondition = NewCondition;
	if (GhostClearedHandle.IsValid())
	{
		UnsubscribeGhostCleared();
	}
	SubscribeGhostCleared();
}

void UInteriorSubsystem::SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition)
{
	if (ItemAcquiredCondition == NewCondition) return;
	ItemAcquiredCondition = NewCondition;
	if (ItemAcquiredHandle.IsValid())
	{
		UnsubscribeItemAcquired();
	}
	SubscribeItemAcquired();
}

void UInteriorSubsystem::SubscribeGhostCleared()
{
	if (GhostClearedHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem: Already subscribed to GhostCleared"));
		return;
	}

	if (!GhostClearedCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem: GhostClearedCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleGhostCleared)
		);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
}

void UInteriorSubsystem::SubscribeItemAcquired()
{
	if (ItemAcquiredHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem: Already subscribed to ItemAcquired"));
		return;
	}

	if (!ItemAcquiredCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem: ItemAcquiredCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		ItemAcquiredHandle = EventBus->RegisterHandler(
			ItemAcquiredCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleItemAcquired)
		);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to ItemAcquired (handle=%u)"), ItemAcquiredHandle.GetId());
	}
}

void UInteriorSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unsubscribed GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
	GhostClearedHandle.Invalidate();
}

void UInteriorSubsystem::UnsubscribeItemAcquired()
{
	if (!ItemAcquiredHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(ItemAcquiredHandle);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unsubscribed ItemAcquired (handle=%u)"), ItemAcquiredHandle.GetId());
	}
	ItemAcquiredHandle.Invalidate();
}

void UInteriorSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeItemAcquired();
}

void UInteriorSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	if (GhostClearedCondition)
	{
		auto Query = GhostClearedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("InteriorSubsystem: Incoming outcome does not satisfy GhostClearedCondition -> ignoring"));
			return;
		}
	}

	if (UGhostClearedPayload* P = Cast<UGhostClearedPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Ghost cleared - Interior: %d"),
			static_cast<int32>(Outcome.OutcomeInterior));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("InteriorSubsystem: HandleGhostCleared called with no payload or unexpected payload type"));
	}

	OnGhostCleared.Broadcast(Outcome);
}

void UInteriorSubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	if (ItemAcquiredCondition)
	{
		auto Query = ItemAcquiredCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("InteriorSubsystem: Incoming outcome does not satisfy ItemAcquiredCondition -> ignoring"));
			return;
		}
	}

	if (UItemAcquiredPayload* P = Cast<UItemAcquiredPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Item acquired - Object: %s"),
			*P->ObjectId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("InteriorSubsystem: HandleItemAcquired called with no payload or unexpected payload type"));
	}

	OnItemAcquired.Broadcast(Outcome);
}

