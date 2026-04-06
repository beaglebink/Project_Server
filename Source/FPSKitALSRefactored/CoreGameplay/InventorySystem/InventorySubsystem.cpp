#include "InventorySubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ItemAcquiredPayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedEventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	// Subscribe interaction registration early
	SubscribeRegistration();

	if (ItemAcquiredCondition) SubscribeItemAcquired();
}

void UInventorySubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UInventorySubsystem::SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition)
{
	if (ItemAcquiredCondition == NewCondition) return;
	ItemAcquiredCondition = NewCondition;
	if (ItemAcquiredHandle.IsValid()) UnsubscribeItemAcquired();
	if (ItemAcquiredCondition) SubscribeItemAcquired();
}

void UInventorySubsystem::SubscribeItemAcquired()
{
	if (ItemAcquiredHandle.IsValid() || !ItemAcquiredCondition) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
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
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get()) EventBus->UnregisterHandler(ItemAcquiredHandle);
	ItemAcquiredHandle.Invalidate();
}

void UInventorySubsystem::SubscribeRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (RegisteredRegisterHandle.IsValid() || UnregisteredRegisterHandle.IsValid()) return;

	// InteractRegistered — Object category
	RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Object;
	RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->FilterRow.ObjectType = EOutcomeObject::InteractRegistered;
	RegisteredConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->CompileCondition();

	// InteractUnregistered — Object category
	UnregisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	UnregisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	UnregisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Object;
	UnregisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	UnregisteredConditionAsset->FilterRow.ObjectType = EOutcomeObject::InteractUnregistered;
	UnregisteredConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
	UnregisteredConditionAsset->CompileCondition();

	if (RegisteredConditionAsset->GetCondition().IsValid())
	{
		RegisteredRegisterHandle = CachedEventBus->RegisterHandler(
			RegisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to InteractRegistered (handle=%u)"), RegisteredRegisterHandle.GetId());
	}

	if (UnregisteredConditionAsset->GetCondition().IsValid())
	{
		UnregisteredRegisterHandle = CachedEventBus->RegisterHandler(
			UnregisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to InteractUnregistered (handle=%u)"), UnregisteredRegisterHandle.GetId());
	}
}

void UInventorySubsystem::UnsubscribeRegistration()
{
	if (!CachedEventBus.IsValid()) return;

	if (RegisteredRegisterHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(RegisteredRegisterHandle);
		RegisteredRegisterHandle.Invalidate();
	}
	if (UnregisteredRegisterHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(UnregisteredRegisterHandle);
		UnregisteredRegisterHandle.Invalidate();
	}

	RegisteredConditionAsset   = nullptr;
	UnregisteredConditionAsset = nullptr;
}

void UInventorySubsystem::UnsubscribeAll()
{
	UnsubscribeItemAcquired();
	UnsubscribeRegistration();
}

void UInventorySubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UInventorySubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

void UInventorySubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	if (UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		AActor* Owner  = P->GetOwnerActor();
		FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

		if (Outcome.OutcomeObject == EOutcomeObject::InteractRegistered)
		{
			FInventoryInteractItemRecord R;
			R.ItemId = Id;
			R.OwnerActor = Owner;
			R.InteractionRange = P->InteractionRange;
			RegisteredItems.Add(Id, R);

			UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Registered interactive item owned by '%s' (range=%.1f)"),
				*OwnerName, P->InteractionRange);

			if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(Id))
			{
				for (auto It = List->CreateIterator(); It; ++It)
				{
					if (UInteractiveItemComponent* Comp = It->Get())
					{
						Comp->OnRegisteredBySubsystem(P);
					}
				}
				RegistrationListeners.Remove(Id);
			}
		}
		else if (Outcome.OutcomeObject == EOutcomeObject::InteractUnregistered)
		{
			FString NameToLog = OwnerName;
			if (RegisteredItems.Contains(Id))
			{
				AActor* RegOwner = RegisteredItems[Id].OwnerActor.Get();
				if (RegOwner) NameToLog = RegOwner->GetName();
				RegisteredItems.Remove(Id);
			}
			else if (NameToLog == "Unknown")
			{
				NameToLog = Id.ToString();
			}

			UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Unregistered interactive item owned by '%s'"), *NameToLog);

			if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(Id))
			{
				for (auto It = List->CreateIterator(); It; ++It)
				{
					if (UInteractiveItemComponent* Comp = It->Get())
					{
						Comp->OnUnregisteredBySubsystem(P);
					}
				}
				RegistrationListeners.Remove(Id);
			}
		}
	}
}

void UInventorySubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	if (ItemAcquiredCondition)
	{
		auto Query = ItemAcquiredCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}
	if (UItemAcquiredPayload* P = Cast<UItemAcquiredPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired - Object: %s"),
			*P->ObjectId.ToString());
	}
	OnItemAcquired.Broadcast(Outcome);
}
