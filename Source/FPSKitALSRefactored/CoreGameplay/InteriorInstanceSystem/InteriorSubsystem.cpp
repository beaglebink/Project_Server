#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "../InventorySystem/ItemAcquiredPayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractItemStatePayload.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	// Subscribe interaction registration events first (so items registering during BeginPlay are handled)
	SubscribeInteractionRegistration();

	// Subscribe other logical events if conditions are provided
	if (CachedEventBus.IsValid())
	{
		if (GhostClearedCondition)
		{
			SubscribeGhostCleared();
		}
		if (ItemAcquiredCondition)
		{
			SubscribeItemAcquired();
		}
	}
}

void UInteriorSubsystem::Deinitialize()
{
	UnsubscribeInteractionRegistration();
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UInteriorSubsystem::SubscribeGhostCleared()
{
	if (!CachedEventBus.IsValid() || !GhostClearedCondition) return;
	if (GhostClearedHandle.IsValid()) return;

	GhostClearedHandle = CachedEventBus->RegisterHandler(
		GhostClearedCondition,
		FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleGhostCleared)
	);

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
}

void UInteriorSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid() || !CachedEventBus.IsValid()) return;
	CachedEventBus->UnregisterHandler(GhostClearedHandle);
	GhostClearedHandle.Invalidate();
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unsubscribed GhostCleared"));
}

void UInteriorSubsystem::SubscribeItemAcquired()
{
	if (!CachedEventBus.IsValid() || !ItemAcquiredCondition) return;
	if (ItemAcquiredHandle.IsValid()) return;

	ItemAcquiredHandle = CachedEventBus->RegisterHandler(
		ItemAcquiredCondition,
		FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleItemAcquired)
	);

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to ItemAcquired (handle=%u)"), ItemAcquiredHandle.GetId());
}

void UInteriorSubsystem::UnsubscribeItemAcquired()
{
	if (!ItemAcquiredHandle.IsValid() || !CachedEventBus.IsValid()) return;
	CachedEventBus->UnregisterHandler(ItemAcquiredHandle);
	ItemAcquiredHandle.Invalidate();
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unsubscribed ItemAcquired"));
}

void UInteriorSubsystem::SubscribeInteractionRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (SpawnRegisterHandle.IsValid() || DespawnRegisterHandle.IsValid()) return;

	// Create runtime condition asset for ObjectSpawned
	SpawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SpawnConditionAsset->OperatorType = EConditionOperator::Composite;
	SpawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Object;
	SpawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SpawnConditionAsset->FilterRow.ObjectType = EOutcomeObject::ObjectSpawned;
	SpawnConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
	SpawnConditionAsset->CompileCondition();

	// Create runtime condition asset for ObjectDespawned
	DespawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	DespawnConditionAsset->OperatorType = EConditionOperator::Composite;
	DespawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Object;
	DespawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	DespawnConditionAsset->FilterRow.ObjectType = EOutcomeObject::ObjectDespawned;
	DespawnConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
	DespawnConditionAsset->CompileCondition();

	// Register handlers
	if (SpawnConditionAsset->GetCondition().IsValid())
	{
		SpawnRegisterHandle = CachedEventBus->RegisterHandler(
			SpawnConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration)
		);
	}

	if (DespawnConditionAsset->GetCondition().IsValid())
	{
		DespawnRegisterHandle = CachedEventBus->RegisterHandler(
			DespawnConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration)
		);
	}
}

void UInteriorSubsystem::UnsubscribeInteractionRegistration()
{
	if (!CachedEventBus.IsValid()) return;

	if (SpawnRegisterHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(SpawnRegisterHandle);
		SpawnRegisterHandle.Invalidate();
	}
	if (DespawnRegisterHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(DespawnRegisterHandle);
		DespawnRegisterHandle.Invalidate();
	}

	SpawnConditionAsset = nullptr;
	DespawnConditionAsset = nullptr;
}

void UInteriorSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	if (!CachedEventBus.IsValid()) return;

	// Expect registration payload
	if (UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		if (Outcome.OutcomeObject == EOutcomeObject::ObjectSpawned)
		{
			FInteractItemRecord R;
			R.ItemId = Id;
			R.SubsystemType = P->SubsystemType;
			R.InteractionRange = P->InteractionRange;
			R.DefaultTooltip = P->DefaultTooltip;
			R.OwnerActor = P->OwnerActor;

			RegisteredItems.Add(Id, R);

			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Registered interactive item %s (range=%.1f)"),
				*Id.ToString(), R.InteractionRange);

			// Publish initial state for this item (enabled by default)
			UInteractItemStatePayload* State = CachedEventBus->CreatePayload<UInteractItemStatePayload>();
			State->Setup(Id, true, R.DefaultTooltip, R.InteractionRange);

			FOutcomeEventBase StateEvent;
			StateEvent.OutcomeType = EOutcomeType::Object;
			StateEvent.OutcomeObject = EOutcomeObject::InteractEnabled;
			StateEvent.Payload = State;

			CachedEventBus->PublishOutcome(StateEvent);
		}
		else if (Outcome.OutcomeObject == EOutcomeObject::ObjectDespawned)
		{
			RegisteredItems.Remove(Id);
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unregistered interactive item %s"), *Id.ToString());
		}
	}
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

// Helpers - currently simple wrappers (kept to satisfy header declarations)
void UInteriorSubsystem::SubscribeAll()
{
	SubscribeGhostCleared();
	SubscribeItemAcquired();
	SubscribeInteractionRegistration();
}

void UInteriorSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeItemAcquired();
	UnsubscribeInteractionRegistration();
}

void UInteriorSubsystem::EvaluateConditions()
{
	// Optional debug helper - implement later if needed.
}

