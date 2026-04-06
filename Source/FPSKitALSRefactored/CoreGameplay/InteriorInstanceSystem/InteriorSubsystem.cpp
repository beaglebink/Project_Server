#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "../InventorySystem/ItemAcquiredPayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractItemStatePayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	SubscribeInteractionRegistration();
	SubscribeInteractCommand();
	SubscribeSetEnabled();

	if (CachedEventBus.IsValid())
	{
		if (GhostClearedCondition) SubscribeGhostCleared();
		if (ItemAcquiredCondition) SubscribeItemAcquired();
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
	if (!CachedEventBus.IsValid() || !GhostClearedCondition || GhostClearedHandle.IsValid()) return;
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
	if (!CachedEventBus.IsValid() || !ItemAcquiredCondition || ItemAcquiredHandle.IsValid()) return;
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

	// Create runtime condition asset for InteractRegistered (Interior-specific)
	SpawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SpawnConditionAsset->OperatorType = EConditionOperator::Composite;
	SpawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	SpawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SpawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractRegistered;
	SpawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	SpawnConditionAsset->CompileCondition();

	// Create runtime condition asset for InteractUnregistered (Interior-specific)
	DespawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	DespawnConditionAsset->OperatorType = EConditionOperator::Composite;
	DespawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	DespawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	DespawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractUnregistered;
	DespawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	DespawnConditionAsset->CompileCondition();

	if (SpawnConditionAsset->GetCondition().IsValid())
	{
		SpawnRegisterHandle = CachedEventBus->RegisterHandler(
			SpawnConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to InteractRegistered (handle=%u)"), SpawnRegisterHandle.GetId());
	}

	if (DespawnConditionAsset->GetCondition().IsValid())
	{
		DespawnRegisterHandle = CachedEventBus->RegisterHandler(
			DespawnConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to InteractUnregistered (handle=%u)"), DespawnRegisterHandle.GetId());
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

	SpawnConditionAsset  = nullptr;
	DespawnConditionAsset = nullptr;
}

void UInteriorSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UInteriorSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	if (!Listener) return;
	if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* Arr = RegistrationListeners.Find(ItemId))
	{
		for (int32 i = Arr->Num() - 1; i >= 0; --i)
		{
			if ((*Arr)[i].Get() == Listener || !(*Arr)[i].IsValid())
			{
				Arr->RemoveAtSwap(i);
			}
		}
		if (Arr->Num() == 0)
		{
			RegistrationListeners.Remove(ItemId);
		}
	}
}

void UInteriorSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	if (!CachedEventBus.IsValid()) return;

	if (UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		AActor* Owner   = P->GetOwnerActor();
		FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

		if (Outcome.OutcomeInterior == EOutcomeInterior::InteractRegistered)
		{
			FInteractItemRecord R;
			R.ItemId           = Id;
			R.SubsystemType    = P->SubsystemType;
			R.InteractionRange = P->InteractionRange;
			R.DefaultTooltip   = P->DefaultTooltip;
			R.OwnerActor       = Owner;

			RegisteredItems.Add(Id, R);

			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Registered interactive item owned by '%s' (range=%.1f)"),
				*OwnerName, R.InteractionRange);

			// Notify only the listener for this ItemId
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
		else if (Outcome.OutcomeInterior == EOutcomeInterior::InteractUnregistered)
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

			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unregistered interactive item owned by '%s'"), *NameToLog);

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

void UInteriorSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	if (GhostClearedCondition)
	{
		auto Query = GhostClearedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}
	if (UGhostClearedPayload* P = Cast<UGhostClearedPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Ghost cleared - Interior: %d"),
			static_cast<int32>(Outcome.OutcomeInterior));
	}
	OnGhostCleared.Broadcast(Outcome);
}

void UInteriorSubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	if (ItemAcquiredCondition)
	{
		auto Query = ItemAcquiredCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}
	if (UItemAcquiredPayload* P = Cast<UItemAcquiredPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Item acquired - Object: %s"),
			*P->ObjectId.ToString());
	}
	OnItemAcquired.Broadcast(Outcome);
}

void UInteriorSubsystem::SubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid()) return;

	InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
	// Фильтруем только по типу Outcome, чтобы получать команды только для Interior
	InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	InteractCommandConditionAsset->CompileCondition();

	if (InteractCommandConditionAsset->GetCondition().IsValid())
	{
		InteractCommandHandle = CachedEventBus->RegisterHandler(
			InteractCommandConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractCommand)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to InteractCommand (handle=%u)"), InteractCommandHandle.GetId());
	}
}

void UInteriorSubsystem::UnsubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(InteractCommandHandle);
		InteractCommandHandle.Invalidate();
	}
	InteractCommandConditionAsset = nullptr;
}

void UInteriorSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	if (const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		// Сначала подсистема находит зарегистрированного владельца по ItemId
		if (const FInteractItemRecord* Rec = RegisteredItems.Find(Id))
		{
			AActor* Owner = Rec->OwnerActor.Get();
			if (Owner)
			{
				// Затем вызывает хелпер, передавая Owner и Picker
				ExecuteInteractCommandOnOwner(Id, Owner, P->Picker);
			}
		}
	}
}

void UInteriorSubsystem::SubscribeAll()
{
	SubscribeGhostCleared();
	SubscribeItemAcquired();
	SubscribeInteractionRegistration();
	SubscribeInteractCommand();
}

void UInteriorSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeItemAcquired();
	UnsubscribeInteractCommand();
	UnsubscribeSetEnabled();
}

void UInteriorSubsystem::SubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || SetEnabledHandle.IsValid()) return;

	SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
	SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetEnabled;
	SetEnabledConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->CompileCondition();

	if (SetEnabledConditionAsset->GetCondition().IsValid())
	{
		SetEnabledHandle = CachedEventBus->RegisterHandler(
			SetEnabledConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetEnabled)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to SetEnabled (handle=%u)"), SetEnabledHandle.GetId());
	}
}

void UInteriorSubsystem::UnsubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || !SetEnabledHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetEnabledHandle);
	SetEnabledHandle.Invalidate();
	SetEnabledConditionAsset = nullptr;
}

void UInteriorSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload))
	{
		if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
		}
	}
}

void UInteriorSubsystem::EvaluateConditions() {}

