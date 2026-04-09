#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractItemStatePayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"
#include "FloorAssignmentComponent.h"
#include "FloorPopulationTypes.h"
#include "EngineUtils.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "FloorPlacementPayload.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	// Сбор размещённых акторов отложен: stub вызывает очистку карты.
	CollectPlacedActorsByInteriorFloor();

	// Подписка на размещение (отдельно от интерактивной регистрации)
	SubscribePlacementRegistration();

	// Зарегистрировать собранные объекты в подсистеме (создаст RegisteredItems и уведомления)
	RegisterPlacedActorsInSubsystem();

	SubscribeInteractionRegistration();
	SubscribeInteractCommand();
	SubscribeSetEnabled();
	SubscribeSetRange();
	SubscribeSetTooltip();
}

void UInteriorSubsystem::Deinitialize()
{
	UnsubscribeInteractionRegistration();
	UnsubscribeAll();
	SpawnedActorsByInteriorFloor.Empty();
	CachedEventBus.Reset();
	Super::Deinitialize();
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

	// also cleanup placement handlers if any
	UnsubscribePlacementRegistration();
}

void UInteriorSubsystem::SubscribePlacementRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (PlacementRegisterHandle.IsValid() || PlacementUnregisterHandle.IsValid()) return;

	PlacementRegisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	PlacementRegisterConditionAsset->OperatorType = EConditionOperator::Composite;
	PlacementRegisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	PlacementRegisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	PlacementRegisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	PlacementRegisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementRegistered;
	PlacementRegisterConditionAsset->CompileCondition();

	PlacementUnregisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	PlacementUnregisterConditionAsset->OperatorType = EConditionOperator::Composite;
	PlacementUnregisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	PlacementUnregisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	PlacementUnregisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	PlacementUnregisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementUnregistered;
	PlacementUnregisterConditionAsset->CompileCondition();

	if (PlacementRegisterConditionAsset->GetCondition().IsValid())
	{
		PlacementRegisterHandle = CachedEventBus->RegisterHandler(
			PlacementRegisterConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to FloorPlacementRegistered (handle=%u)"), PlacementRegisterHandle.GetId());
	}

	if (PlacementUnregisterConditionAsset->GetCondition().IsValid())
	{
		PlacementUnregisterHandle = CachedEventBus->RegisterHandler(
			PlacementUnregisterConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to FloorPlacementUnregistered (handle=%u)"), PlacementUnregisterHandle.GetId());
	}
}

void UInteriorSubsystem::UnsubscribePlacementRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (PlacementRegisterHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(PlacementRegisterHandle);
		PlacementRegisterHandle.Invalidate();
	}
	if (PlacementUnregisterHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(PlacementUnregisterHandle);
		PlacementUnregisterHandle.Invalidate();
	}
	PlacementRegisterConditionAsset = nullptr;
	PlacementUnregisterConditionAsset = nullptr;
}

// Separate handler: only processes UFloorPlacementPayload (placed actors). Does NOT touch RegisteredItems.
void UInteriorSubsystem::HandlePlacementRegistration(const FOutcomeEventBase& Outcome)
{
	if (!CachedEventBus.IsValid()) return;

	if (UFloorPlacementPayload* FP = Cast<UFloorPlacementPayload>(Outcome.Payload))
	{
		AActor* Owner = FP->OwnerActor.Get();
		FInteriorFloorKey Key(FP->InteriorSetId, FP->FloorId);

		if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementRegistered)
		{
			FFloorPopulationBuckets& Buckets = SpawnedActorsByInteriorFloor.FindOrAdd(Key);

			// Build record once (store full transform)
			FFloorPopulationRecord NewRec;
			NewRec.ActorType = FP->ActorType;
			NewRec.SourceClass = Owner ? Owner->GetClass() : nullptr;
			NewRec.PlacedActor = Owner;
			NewRec.AnchorId = FP->AnchorId;
			NewRec.WorldTransform = FP->WorldTransform;
			NewRec.bHasAnchor = FP->AnchorId.IsValid();

			// Helper lambda: check duplicate in array (owner, anchor, or identical transform if no actor)
			auto ExistsInArray = [&](const TArray<FFloorPopulationRecord>& Arr)->bool
			{
				for (const FFloorPopulationRecord& R : Arr)
				{
					if (R.PlacedActor.IsValid() && Owner && R.PlacedActor.Get() == Owner) return true;
					if (FP->AnchorId.IsValid() && R.AnchorId == FP->AnchorId) return true;
					// legacy check: if there's no placed actor, compare transforms
					if (!R.PlacedActor.IsValid() && R.WorldTransform.Equals(FP->WorldTransform)) return true;
				}
				return false;
			};

			bool bAdded = false;
			switch (FP->ActorType)
			{
			case EFloorActorType::HeavyFurniture:
				if (!ExistsInArray(Buckets.HeavyFurniture)) { Buckets.HeavyFurniture.Add(NewRec); bAdded = true; }
				break;
			case EFloorActorType::LightItem:
				if (!ExistsInArray(Buckets.LightItems)) { Buckets.LightItems.Add(NewRec); bAdded = true; }
				break;
			case EFloorActorType::Terminal:
				if (!ExistsInArray(Buckets.Terminals)) { Buckets.Terminals.Add(NewRec); bAdded = true; }
				break;
			case EFloorActorType::NPC_Spawner:
				if (!ExistsInArray(Buckets.NPCSpawners)) { Buckets.NPCSpawners.Add(NewRec); bAdded = true; }
				break;
			case EFloorActorType::Debris:
				if (!ExistsInArray(Buckets.Debris)) { Buckets.Debris.Add(NewRec); bAdded = true; }
				break;
			default:
				if (!ExistsInArray(Buckets.LightItems)) { Buckets.LightItems.Add(NewRec); bAdded = true; }
				break;
			}

			if (bAdded)
			{
				UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placed actor registered Owner=%s Interior=%s Floor=%s Type=%d"),
					Owner ? *Owner->GetName() : TEXT("None"),
					*FP->InteriorSetId.ToString(), *FP->FloorId.ToString(), static_cast<int32>(FP->ActorType));
			}
		}
		else if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementUnregistered)
		{
			if (FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
			{
				auto RemoveMatches = [&](TArray<FFloorPopulationRecord>& Arr)
				{
					for (int32 i = Arr.Num() - 1; i >= 0; --i)
					{
						const FFloorPopulationRecord& R = Arr[i];
						if ((R.PlacedActor.IsValid() && Owner && R.PlacedActor.Get() == Owner) ||
							(R.AnchorId.IsValid() && R.AnchorId == FP->AnchorId) ||
							(!R.PlacedActor.IsValid() && R.WorldTransform.Equals(FP->WorldTransform)))
						{
							Arr.RemoveAtSwap(i);
						}
					}
				};

				RemoveMatches(Buckets->HeavyFurniture);
				RemoveMatches(Buckets->LightItems);
				RemoveMatches(Buckets->Terminals);
				RemoveMatches(Buckets->NPCSpawners);
				RemoveMatches(Buckets->Debris);

				// remove entire key if all buckets empty
				if (Buckets->HeavyFurniture.Num() == 0 &&
					Buckets->LightItems.Num() == 0 &&
					Buckets->Terminals.Num() == 0 &&
					Buckets->NPCSpawners.Num() == 0 &&
					Buckets->Debris.Num() == 0)
				{
					SpawnedActorsByInteriorFloor.Remove(Key);
				}
			}
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placed actor unregistered Owner=%s"), Owner ? *Owner->GetName() : TEXT("None"));
		}
	}
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

	// Only handle interactive registration payloads here (UInteractItemRegistrationPayload).
	// Floor placement payloads are handled by separate handler HandlePlacementRegistration.
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
	// GhostCleared / ItemAcquired handlers были удалены — больше не подписываемся на них.
	SubscribeInteractionRegistration();
	SubscribeInteractCommand();
}

void UInteriorSubsystem::UnsubscribeAll()
{
	// GhostCleared / ItemAcquired handlers были удалены — больше не отписываемся от них.
	UnsubscribeInteractCommand();
	UnsubscribeSetEnabled();
	UnsubscribeSetRange();
	UnsubscribeSetTooltip();
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

void UInteriorSubsystem::SubscribeSetRange()
{
	if (!CachedEventBus.IsValid() || SetRangeHandle.IsValid()) return;

	SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
	SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetRangeConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetRange;
	SetRangeConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	SetRangeConditionAsset->CompileCondition();

	if (SetRangeConditionAsset->GetCondition().IsValid())
	{
		SetRangeHandle = CachedEventBus->RegisterHandler(
			SetRangeConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetRange)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to SetRange (handle=%u)"), SetRangeHandle.GetId());
	}
}

void UInteriorSubsystem::UnsubscribeSetRange()
{
	if (!CachedEventBus.IsValid() || !SetRangeHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetRangeHandle);
	SetRangeHandle.Invalidate();
	SetRangeConditionAsset = nullptr;
}

void UInteriorSubsystem::SubscribeSetTooltip()
{
	if (!CachedEventBus.IsValid() || SetTooltipHandle.IsValid()) return;

	SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
	SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
	SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetTooltipConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetTooltip;
	SetTooltipConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
	SetTooltipConditionAsset->CompileCondition();

	if (SetTooltipConditionAsset->GetCondition().IsValid())
	{
		SetTooltipHandle = CachedEventBus->RegisterHandler(
			SetTooltipConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetTooltip)
		);
		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to SetTooltip (handle=%u)"), SetTooltipHandle.GetId());
	}
}

void UInteriorSubsystem::UnsubscribeSetTooltip()
{
	if (!CachedEventBus.IsValid() || !SetTooltipHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetTooltipHandle);
	SetTooltipHandle.Invalidate();
	SetTooltipConditionAsset = nullptr;
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

void UInteriorSubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload))
	{
		if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
		}
	}
}

void UInteriorSubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload))
	{
		if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
		}
	}
}

void UInteriorSubsystem::EvaluateConditions() {}

// -----------------------------------------------------------------------------
// Stub: сбор размещённых акторов отложен — пока простой очиститель карты.
// -----------------------------------------------------------------------------
// TODO: implement detailed collection logic later (mapping placed actors to InteriorSet+Floor)
void UInteriorSubsystem::CollectPlacedActorsByInteriorFloor()
{
	// TODO: implement detailed collection logic later (mapping placed actors to InteriorSet+Floor)
	SpawnedActorsByInteriorFloor.Empty();
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: CollectPlacedActorsByInteriorFloor() is stubbed — implementation deferred."));
}

// Map floor actor type -> interactive subsystem used for payload/registration
static EInteractiveSubsystem MapFloorActorTypeToInteractiveSubsystem(EFloorActorType T)
{
	switch (T)
	{
	case EFloorActorType::Terminal:
		return EInteractiveSubsystem::Terminal;
	case EFloorActorType::NPC_Spawner:
		return EInteractiveSubsystem::ActorNPC;
	case EFloorActorType::Debris:
		// Debris is passive interior item — handled by Interior subsystem
		return EInteractiveSubsystem::Interior;
	case EFloorActorType::HeavyFurniture:
	case EFloorActorType::LightItem:
	default:
		return EInteractiveSubsystem::Interior;
	}
}

void UInteriorSubsystem::RegisterPlacedActorsInSubsystem()
{
	if (SpawnedActorsByInteriorFloor.Num() == 0)
	{
		return;
	}

	int32 RegisteredCountBefore = RegisteredItems.Num();

	// Helper to process one array of population records
	auto ProcessArray = [&](const TArray<FFloorPopulationRecord>& Arr)
	{
		for (const FFloorPopulationRecord& Rec : Arr)
		{
			AActor* Owner = Rec.PlacedActor.Get();
			if (!IsValid(Owner))
			{
				continue;
			}

			// Avoid duplicate registration by owner pointer
			bool bOwnerAlreadyRegistered = false;
			for (const auto& KV : RegisteredItems)
			{
				const FInteractItemRecord& Existing = KV.Value;
				if (Existing.OwnerActor.IsValid() && Existing.OwnerActor.Get() == Owner)
				{
					bOwnerAlreadyRegistered = true;
					break;
				}
			}
			if (bOwnerAlreadyRegistered)
			{
				continue;
			}

			// Prefer interactive component if present
			UInteractiveItemComponent* InterComp = Owner->FindComponentByClass<UInteractiveItemComponent>();

			FGuid ItemId;
			EInteractiveSubsystem Subsystem = EInteractiveSubsystem::Interior;
			float InteractionRange = 200.f;
			FText DefaultTooltip = FText::GetEmpty();

			if (InterComp)
			{
				// Use component-provided stable id and settings
				ItemId = InterComp->GetItemId();
				Subsystem = InterComp->SubsystemType;
				InteractionRange = InterComp->InteractionRange;
				DefaultTooltip = InterComp->InteractiveTooltipText;

				// If component has no ItemId (unlikely) — fallback to assignment component or new GUID
				if (!ItemId.IsValid())
				{
					if (UFloorAssignmentComponent* Assign = Owner->FindComponentByClass<UFloorAssignmentComponent>())
					{
						if (Assign->ItemId.IsValid())
						{
							ItemId = Assign->ItemId;
						}
						else
						{
							ItemId = FGuid::NewGuid();
							Assign->Modify();
							Assign->ItemId = ItemId;
							if (UPackage* Pkg = Assign->GetOwner()->GetOutermost())
							{
								Pkg->MarkPackageDirty();
							}
						}
					}
					else
					{
						ItemId = FGuid::NewGuid();
					}
				}
			}
			else
			{
				// No interactive component — use ActorType from Rec to decide if we should register
				Subsystem = MapFloorActorTypeToInteractiveSubsystem(Rec.ActorType);
				// Treat Interior mapping as non-explicit interactive (skip)
				if (Subsystem == EInteractiveSubsystem::Interior)
				{
					continue; // do not register passive interior-only population items
				}

				// Get stable id from FloorAssignmentComponent if possible
				if (UFloorAssignmentComponent* Assign = Owner->FindComponentByClass<UFloorAssignmentComponent>())
				{
					if (Assign->ItemId.IsValid())
					{
						ItemId = Assign->ItemId;
					}
					else
					{
						ItemId = FGuid::NewGuid();
						Assign->Modify();
						Assign->ItemId = ItemId;
						if (UPackage* Pkg = Assign->GetOwner()->GetOutermost())
						{
							Pkg->MarkPackageDirty();
						}
					}
				}
				else
				{
					ItemId = FGuid::NewGuid();
				}
			}

			// Prevent duplicate registration by ItemId
			if (RegisteredItems.Contains(ItemId))
			{
				continue;
			}

			// Build internal record
			FInteractItemRecord R;
			R.ItemId = ItemId;
			R.SubsystemType = Subsystem;
			R.InteractionRange = InteractionRange;
			R.DefaultTooltip = DefaultTooltip;
			R.OwnerActor = Owner;

			RegisteredItems.Add(ItemId, R);

			// Publish Outcome via EventBus
			if (CachedEventBus.IsValid())
			{
				UInteractItemRegistrationPayload* P = CachedEventBus->CreatePayload<UInteractItemRegistrationPayload>();
				if (P)
				{
					P->Setup(ItemId, R.SubsystemType, R.InteractionRange, R.DefaultTooltip, Owner);

					FOutcomeEventBase Out;
					Out.OutcomeType = EOutcomeType::Interior;
					Out.OutcomeInterior = EOutcomeInterior::InteractRegistered;
					Out.Payload = P;

					CachedEventBus->PublishOutcome(Out);
				}
			}
			else
			{
				// Fallback for tools/legacy: broadcast delegate
				UInteractItemRegistrationPayload* P = NewObject<UInteractItemRegistrationPayload>(this);
				if (P)
				{
					P->Setup(ItemId, R.SubsystemType, R.InteractionRange, R.DefaultTooltip, Owner);
					OnInteractItemRegistered.Broadcast(P);
				}
			}

			// Notify any listeners waiting for this item (by ItemId)
			if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(ItemId))
			{
				for (auto It = List->CreateIterator(); It; ++It)
				{
					if (UInteractiveItemComponent* Comp = It->Get())
					{
						UInteractItemRegistrationPayload* TmpP = NewObject<UInteractItemRegistrationPayload>(this);
						if (TmpP)
						{
							TmpP->Setup(ItemId, R.SubsystemType, R.InteractionRange, R.DefaultTooltip, Owner);
							Comp->OnRegisteredBySubsystem(TmpP);
						}
					}
				}
				RegistrationListeners.Remove(ItemId);
			}
		}
	};

	// Iterate all buckets and process each semantic array separately
	for (const auto& Pair : SpawnedActorsByInteriorFloor)
	{
		const FFloorPopulationBuckets& Buckets = Pair.Value;
		ProcessArray(Buckets.HeavyFurniture);
		ProcessArray(Buckets.LightItems);
		ProcessArray(Buckets.Terminals);
		ProcessArray(Buckets.NPCSpawners);
		ProcessArray(Buckets.Debris);
	}

	int32 RegisteredCountAfter = RegisteredItems.Num() - RegisteredCountBefore;
	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Registered %d new placed actors into RegisteredItems."), RegisteredCountAfter);
}

TArray<FFloorPopulationRecord> UInteriorSubsystem::GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const
{
	FInteriorFloorKey Key(InteriorSetId, FloorId);
	if (const FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
	{
		// Return aggregated array (caller can inspect buckets separately via new getter if needed)
		TArray<FFloorPopulationRecord> Combined;
		Combined.Append(Buckets->HeavyFurniture);
		Combined.Append(Buckets->LightItems);
		Combined.Append(Buckets->Terminals);
		Combined.Append(Buckets->NPCSpawners);
		Combined.Append(Buckets->Debris);
		return Combined;
	}
	return TArray<FFloorPopulationRecord>();
}

FFloorPopulationBuckets UInteriorSubsystem::GetPopulationBucketsForFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const
{
	FInteriorFloorKey Key(InteriorSetId, FloorId);
	if (const FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
	{
		return *Buckets;
	}
	return FFloorPopulationBuckets();
}
