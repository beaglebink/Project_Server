#include "InteriorSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractItemStatePayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"
#include "FloorStatePayload.h"
#include "FloorAssignmentComponent.h"
#include "UObject/UnrealType.h"
#include "EngineUtils.h"
// AssetRegistry + module manager for runtime asset indexing
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../LocationSystem/FloorAsset.h"

// -----------------------------------------------------------------------------
// Snapshot helpers
// -----------------------------------------------------------------------------
namespace
{
	// Собрать SaveGame-свойства любого UObject в массив
	static void CollectSaveGameProperties(UObject* Obj, TArray<FFloorSavedPropertyEntry>& OutProps)
	{
		if (!IsValid(Obj)) return;
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop->HasAllPropertyFlags(CPF_SaveGame)) continue;

			FString Exported;
			Prop->ExportText_InContainer(0, Exported, Obj, nullptr, Obj, PPF_None);
			OutProps.Add({ Prop->GetFName(), MoveTemp(Exported) });
		}
	}

	// Применить сохранённые SaveGame-свойства обратно к UObject
	static void ApplySaveGameProperties(UObject* Obj, const TArray<FFloorSavedPropertyEntry>& Props)
	{
		if (!IsValid(Obj)) return;
		for (const FFloorSavedPropertyEntry& Entry : Props)
		{
			FProperty* Prop = FindFProperty<FProperty>(Obj->GetClass(), Entry.PropertyName);
			if (!Prop) continue;
			void* Container = Prop->ContainerPtrToValuePtr<void>(Obj);
			Prop->ImportText_InContainer(*Entry.ValueText, Obj, Obj, PPF_None);
		}
	}
}

// -----------------------------------------------------------------------------
// SaveFloorActorsState
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId)
{
	UWorld* W = GetWorld();
	if (!W) return;

	FInteriorFloorKey Key(InteriorSetId, FloorId);
	TArray<FFloorSavedActorState>& Bucket = FloorStateSnapshots.FindOrAdd(Key);

	// Собираем множество уже сохранённых ItemId, чтобы не дублировать
	TSet<FGuid> AlreadySaved;
	for (const FFloorSavedActorState& S : Bucket)
	{
		AlreadySaved.Add(S.ItemId);
	}

	int32 NewCount = 0;
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;

		UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
		if (!Comp) continue;
		if (Comp->GetInteriorSetId() != InteriorSetId || Comp->GetFloorId() != FloorId) continue;

		// Не добавляем дублирующую запись для того же ItemId
		if (AlreadySaved.Contains(Comp->ItemId)) continue;

		FFloorSavedActorState Snapshot;
		Snapshot.ItemId = Comp->ItemId;
		Snapshot.ActorTransform = Actor->GetActorTransform();

		// Actor SaveGame properties
		CollectSaveGameProperties(Actor, Snapshot.ActorProperties);

		// Component SaveGame properties
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* C : Components)
		{
			if (!IsValid(C)) continue;
			TArray<FFloorSavedPropertyEntry> Props;
			CollectSaveGameProperties(C, Props);
			if (Props.Num() > 0)
			{
				FFloorSavedComponentState CState;
				CState.ComponentName      = C->GetFName();
				CState.ComponentClassName = C->GetClass()->GetFName();
				CState.Properties         = MoveTemp(Props);
				Snapshot.ComponentStates.Add(MoveTemp(CState));
			}
		}

		Bucket.Add(MoveTemp(Snapshot));
		++NewCount;
	}

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: SaveFloorActorsState — added %d new snapshots for floor %s/%s (total: %d)"),
		NewCount, *InteriorSetId.ToString(), *FloorId.ToString(), Bucket.Num());
}

// -----------------------------------------------------------------------------
// RestoreFloorActorsState
// -----------------------------------------------------------------------------

int32 UInteriorSubsystem::RestoreFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId)
{
	UWorld* W = GetWorld();
	if (!W) return 0;

	FInteriorFloorKey Key(InteriorSetId, FloorId);
	const TArray<FFloorSavedActorState>* Found = FloorStateSnapshots.Find(Key);
	if (!Found || Found->IsEmpty()) return 0;

	// Индексируем акторы в мире по ItemId для быстрого поиска
	TMap<FGuid, AActor*> ActorByItemId;
	for (TActorIterator<AActor> It(W); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor)) continue;
		if (UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>())
		{
			ActorByItemId.Add(Comp->ItemId, Actor);
		}
	}

	int32 Restored = 0;
	for (const FFloorSavedActorState& Snapshot : *Found)
	{
		AActor** ActorPtr = ActorByItemId.Find(Snapshot.ItemId);
		if (!ActorPtr || !IsValid(*ActorPtr)) continue;
		AActor* Actor = *ActorPtr;

		// Восстановить Transform
		Actor->SetActorTransform(Snapshot.ActorTransform, false, nullptr, ETeleportType::TeleportPhysics);

		// Восстановить SaveGame-свойства актора
		ApplySaveGameProperties(Actor, Snapshot.ActorProperties);

		// Восстановить SaveGame-свойства компонентов
		for (const FFloorSavedComponentState& CState : Snapshot.ComponentStates)
		{
			UActorComponent* Target = nullptr;
			for (UActorComponent* C : Actor->GetComponents())
			{
				if (!IsValid(C)) continue;
				if (C->GetFName() == CState.ComponentName ||
				    C->GetClass()->GetFName() == CState.ComponentClassName)
				{
					Target = C;
					break;
				}
			}
			if (Target)
			{
				ApplySaveGameProperties(Target, CState.Properties);
			}
		}

		++Restored;
	}

	UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: RestoreFloorActorsState — restored %d actors for floor %s/%s"),
		Restored, *InteriorSetId.ToString(), *FloorId.ToString());

	return Restored;
}

// -----------------------------------------------------------------------------
// EventBus handlers — Registration
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload);
	if (!P) return;

	AActor* Owner = P->GetOwnerActor();
	const FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

	if (Outcome.OutcomeInterior == EOutcomeInterior::InteractRegistered)
	{
		FInteractItemRecord R;
		R.ItemId           = P->ItemId;
		R.OwnerActor       = Owner;
		R.InteractionRange = P->InteractionRange;
		R.SubsystemType    = P->SubsystemType;
		RegisteredItems.Add(P->ItemId, R);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Registered item '%s' (range=%.1f)"), *OwnerName, P->InteractionRange);

		if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
		{
			for (auto It = List->CreateIterator(); It; ++It)
			{
				if (UInteractiveItemComponent* Comp = It->Get())
				{
					Comp->OnRegisteredBySubsystem(P);
				}
			}
			RegistrationListeners.Remove(P->ItemId);
		}

		OnInteractItemRegistered.Broadcast(P);
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::InteractUnregistered)
	{
		RegisteredItems.Remove(P->ItemId);

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Unregistered item '%s'"), *OwnerName);

		if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
		{
			for (auto It = List->CreateIterator(); It; ++It)
			{
				if (UInteractiveItemComponent* Comp = It->Get())
				{
					Comp->OnUnregisteredBySubsystem(P);
				}
			}
			RegistrationListeners.Remove(P->ItemId);
		}

		OnInteractItemUnregistered.Broadcast(P);
	}
}

void UInteriorSubsystem::HandlePlacementRegistration(const FOutcomeEventBase& Outcome)
{
	UFloorPlacementPayload* P = Cast<UFloorPlacementPayload>(Outcome.Payload);
	if (!P) return;

	FInteriorFloorKey Key(P->InteriorSetId, P->FloorId);

	if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementRegistered)
	{
		// Собираем запись из полей payload
		FFloorPopulationRecord NewRecord;
		NewRecord.ActorType      = P->ActorType;
		NewRecord.AnchorId       = P->AnchorId;
		NewRecord.WorldTransform = P->WorldTransform;
		NewRecord.PlacedActor    = P->OwnerActor.Get();
		NewRecord.bHasAnchor     = P->AnchorId.IsValid();

		FFloorPopulationBuckets& Buckets = SpawnedActorsByInteriorFloor.FindOrAdd(Key);

		// Раскладываем по нужному бакету согласно типу
		switch (P->ActorType)
		{
		case EFloorActorType::HeavyFurniture: Buckets.HeavyFurniture.Add(MoveTemp(NewRecord)); break;
		case EFloorActorType::LightItem:      Buckets.LightItems.Add(MoveTemp(NewRecord));      break;
		case EFloorActorType::Terminal:       Buckets.Terminals.Add(MoveTemp(NewRecord));       break;
		case EFloorActorType::NPC_Spawner:    Buckets.NPCSpawners.Add(MoveTemp(NewRecord));     break;
		case EFloorActorType::Debris:         Buckets.Debris.Add(MoveTemp(NewRecord));          break;
		default:                              Buckets.LightItems.Add(MoveTemp(NewRecord));      break;
		}

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placement registered for floor %s/%s (AnchorId=%s)"),
			*P->InteriorSetId.ToString(), *P->FloorId.ToString(), *P->AnchorId.ToString());
	}
	else if (Outcome.OutcomeInterior == EOutcomeInterior::FloorPlacementUnregistered)
	{
		if (FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
		{
			// Удаляем по AnchorId — единственный стабильный идентификатор в FFloorPopulationRecord
			const FGuid AnchorToRemove = P->AnchorId;
			auto RemoveByAnchor = [&AnchorToRemove](TArray<FFloorPopulationRecord>& Arr)
			{
				Arr.RemoveAll([&AnchorToRemove](const FFloorPopulationRecord& R)
				{
					return R.AnchorId == AnchorToRemove;
				});
			};

			RemoveByAnchor(Buckets->HeavyFurniture);
			RemoveByAnchor(Buckets->LightItems);
			RemoveByAnchor(Buckets->Terminals);
			RemoveByAnchor(Buckets->NPCSpawners);
			RemoveByAnchor(Buckets->Debris);
		}

		UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Placement unregistered for floor %s/%s (AnchorId=%s)"),
			*P->InteriorSetId.ToString(), *P->FloorId.ToString(), *P->AnchorId.ToString());
	}
}

// -----------------------------------------------------------------------------
// EventBus handlers — Interact commands
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload);
	if (!P) return;

	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
	{
		AActor* Owner = Rec->OwnerActor.Get();
		if (Owner)
		{
			ExecuteInteractCommandOnOwner(P->ItemId, Owner, P->Picker);
		}
	}
}

void UInteriorSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload);
	if (!P) return;

	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
	{
		ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
	}
}

void UInteriorSubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
	const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload);
	if (!P) return;

	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
	{
		ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
	}
}

void UInteriorSubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
	const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload);
	if (!P) return;

	if (const FInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
	{
		ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
	}
}

// -----------------------------------------------------------------------------
// EventBus handlers — Floor snapshot
// -----------------------------------------------------------------------------

void UInteriorSubsystem::HandleFloorStateSave(const FOutcomeEventBase& Outcome)
{
	if (UFloorStatePayload* P = Cast<UFloorStatePayload>(Outcome.Payload))
	{
		SaveFloorActorsState(P->InteriorSetId, P->FloorId);
	}
}

void UInteriorSubsystem::HandleFloorStateRestore(const FOutcomeEventBase& Outcome)
{
	if (UFloorStatePayload* P = Cast<UFloorStatePayload>(Outcome.Payload))
	{
		RestoreFloorActorsState(P->InteriorSetId, P->FloorId);
	}
}

// -----------------------------------------------------------------------------
// Subscribe / Unsubscribe — полная реализация
// -----------------------------------------------------------------------------

void UInteriorSubsystem::SubscribeAll()
{
	// NOTE: Do not call Super::SubscribeAll() - UWorldSubsystem doesn't implement it.
	UEventBusSubsystem* EventBus = CachedEventBus.Get();
	if (!EventBus) return;

	// ── Registration ──────────────────────────────────────────────────────────
	if (!SpawnRegisterHandle.IsValid())
	{
		SpawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SpawnConditionAsset->OperatorType = EConditionOperator::Composite;
		SpawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SpawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SpawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractRegistered;
		SpawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SpawnConditionAsset->CompileCondition();

		if (SpawnConditionAsset->GetCondition().IsValid())
		{
			SpawnRegisterHandle = EventBus->RegisterHandler(
				SpawnConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to InteractRegistered (handle=%u)"), SpawnRegisterHandle.GetId());
		}
	}

	if (!DespawnRegisterHandle.IsValid())
	{
		DespawnConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		DespawnConditionAsset->OperatorType = EConditionOperator::Composite;
		DespawnConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		DespawnConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		DespawnConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractUnregistered;
		DespawnConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		DespawnConditionAsset->CompileCondition();

		if (DespawnConditionAsset->GetCondition().IsValid())
		{
			DespawnRegisterHandle = EventBus->RegisterHandler(
				DespawnConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractRegistration));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to InteractUnregistered (handle=%u)"), DespawnRegisterHandle.GetId());
		}
	}

	// ── Placement registration ─────────────────────────────────────────────
	SubscribePlacementRegistration();

	// ── InteractCommand ───────────────────────────────────────────────────
	if (!InteractCommandHandle.IsValid())
	{
		InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
		InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		InteractCommandConditionAsset->CompileCondition();

		if (InteractCommandConditionAsset->GetCondition().IsValid())
		{
			InteractCommandHandle = EventBus->RegisterHandler(
				InteractCommandConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleInteractCommand));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to InteractCommand (handle=%u)"), InteractCommandHandle.GetId());
		}
	}

	// ── SetEnabled ────────────────────────────────────────────────────────
	if (!SetEnabledHandle.IsValid())
	{
		SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
		SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetEnabledConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetEnabled;
		SetEnabledConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetEnabledConditionAsset->CompileCondition();

		if (SetEnabledConditionAsset->GetCondition().IsValid())
		{
			SetEnabledHandle = EventBus->RegisterHandler(
				SetEnabledConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetEnabled));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to SetEnabled (handle=%u)"), SetEnabledHandle.GetId());
		}
	}

	// ── SetRange ──────────────────────────────────────────────────────────
	if (!SetRangeHandle.IsValid())
	{
		SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
		SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetRangeConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetRange;
		SetRangeConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetRangeConditionAsset->CompileCondition();

		if (SetRangeConditionAsset->GetCondition().IsValid())
		{
			SetRangeHandle = EventBus->RegisterHandler(
				SetRangeConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetRange));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to SetRange (handle=%u)"), SetRangeHandle.GetId());
		}
	}

	// ── SetTooltip ────────────────────────────────────────────────────────
	if (!SetTooltipHandle.IsValid())
	{
		SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
		SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		SetTooltipConditionAsset->FilterRow.InteriorType = EOutcomeInterior::InteractSetTooltip;
		SetTooltipConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		SetTooltipConditionAsset->CompileCondition();

		if (SetTooltipConditionAsset->GetCondition().IsValid())
		{
			SetTooltipHandle = EventBus->RegisterHandler(
				SetTooltipConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleSetTooltip));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to SetTooltip (handle=%u)"), SetTooltipHandle.GetId());
		}
	}

	// ── FloorStateSave / FloorStateRestore ────────────────────────────────
	if (!FloorStateSaveHandle.IsValid())
	{
		FloorStateSaveConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorStateSaveConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorStateSaveConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorStateSaveConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorStateSaveConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorStateSave;
		FloorStateSaveConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorStateSaveConditionAsset->CompileCondition();

		if (FloorStateSaveConditionAsset->GetCondition().IsValid())
		{
			FloorStateSaveHandle = EventBus->RegisterHandler(
				FloorStateSaveConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorStateSave));
		}
	}

	if (!FloorStateRestoreHandle.IsValid())
	{
		FloorStateRestoreConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		FloorStateRestoreConditionAsset->OperatorType = EConditionOperator::Composite;
		FloorStateRestoreConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		FloorStateRestoreConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		FloorStateRestoreConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorStateRestore;
		FloorStateRestoreConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		FloorStateRestoreConditionAsset->CompileCondition();

		if (FloorStateRestoreConditionAsset->GetCondition().IsValid())
		{
			FloorStateRestoreHandle = EventBus->RegisterHandler(
				FloorStateRestoreConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandleFloorStateRestore));
		}
	}
}

void UInteriorSubsystem::SubscribePlacementRegistration()
{
	UEventBusSubsystem* EventBus = CachedEventBus.Get();
	if (!EventBus) return;

	if (!PlacementRegisterHandle.IsValid())
	{
		PlacementRegisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		PlacementRegisterConditionAsset->OperatorType = EConditionOperator::Composite;
		PlacementRegisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		PlacementRegisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		PlacementRegisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementRegistered;
		PlacementRegisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		PlacementRegisterConditionAsset->CompileCondition();

		if (PlacementRegisterConditionAsset->GetCondition().IsValid())
		{
			PlacementRegisterHandle = EventBus->RegisterHandler(
				PlacementRegisterConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to FloorPlacementRegistered (handle=%u)"), PlacementRegisterHandle.GetId());
		}
	}

	if (!PlacementUnregisterHandle.IsValid())
	{
		PlacementUnregisterConditionAsset = NewObject<UOutcomeConditionAsset>(this);
		PlacementUnregisterConditionAsset->OperatorType = EConditionOperator::Composite;
		PlacementUnregisterConditionAsset->FilterRow.OutcomeType = EOutcomeType::Interior;
		PlacementUnregisterConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
		PlacementUnregisterConditionAsset->FilterRow.InteriorType = EOutcomeInterior::FloorPlacementUnregistered;
		PlacementUnregisterConditionAsset->FilterRow.InteriorComparison = EConditionComparison::Equals;
		PlacementUnregisterConditionAsset->CompileCondition();

		if (PlacementUnregisterConditionAsset->GetCondition().IsValid())
		{
			PlacementUnregisterHandle = EventBus->RegisterHandler(
				PlacementUnregisterConditionAsset,
				FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::HandlePlacementRegistration));
			UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Subscribed to FloorPlacementUnregistered (handle=%u)"), PlacementUnregisterHandle.GetId());
		}
	}
}

void UInteriorSubsystem::UnsubscribePlacementRegistration()
{
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		auto Unreg = [&](FOutcomeHandlerHandle& Handle, UOutcomeConditionAsset*& Asset)
		{
			if (Handle.IsValid()) { EventBus->UnregisterHandler(Handle); Handle.Invalidate(); }
			Asset = nullptr;
		};
		Unreg(PlacementRegisterHandle,   PlacementRegisterConditionAsset);
		Unreg(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);
	}
}

// ===== missing method implementations (stubs with minimal correct behavior) =====

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	// Cache EventBus and subscribe handlers
	if (UGameInstance* GI = InWorld.GetGameInstance())
	{
		CachedEventBus = GI->GetSubsystem<UEventBusSubsystem>();
	}
	// Build runtime asset index (friendly keys) once and subscribe handlers
	BuildAssetIndex();
	SubscribeAll();
}

void UInteriorSubsystem::Deinitialize()
{
	// Unsubscribe everything and clear caches
	UnsubscribeAll();

	// Clear runtime registries
	RegisteredItems.Empty();
	RegistrationListeners.Empty();
	SpawnedActorsByInteriorFloor.Empty();
	FloorStateSnapshots.Empty();

	CachedEventBus = nullptr;

	Super::Deinitialize();
}

// Simple wrappers — delegate to SubscribeAll/UnsubscribeAll
void UInteriorSubsystem::SubscribeInteractionRegistration() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeInteractionRegistration() { UnsubscribeAll(); }
void UInteriorSubsystem::SubscribeInteractCommand() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeInteractCommand() { UnsubscribeAll(); }
void UInteriorSubsystem::SubscribeSetEnabled() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetEnabled() { UnsubscribeAll(); }
void UInteriorSubsystem::SubscribeSetRange() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetRange() { UnsubscribeAll(); }
void UInteriorSubsystem::SubscribeSetTooltip() { SubscribeAll(); }
void UInteriorSubsystem::UnsubscribeSetTooltip() { UnsubscribeAll(); }

// Add/remove per-item registration listeners
void UInteriorSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UInteriorSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

// Возвращает объединённый список всех записей в buckets для ключа
TArray<FFloorPopulationRecord> UInteriorSubsystem::GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const
{
	TArray<FFloorPopulationRecord> Result;
	const FInteriorFloorKey Key(InteriorSetId, FloorId);
	if (const FFloorPopulationBuckets* Buckets = SpawnedActorsByInteriorFloor.Find(Key))
	{
		Result.Append(Buckets->HeavyFurniture);
		Result.Append(Buckets->LightItems);
		Result.Append(Buckets->Terminals);
		Result.Append(Buckets->NPCSpawners);
		Result.Append(Buckets->Debris);
	}
	return Result;
}

void UInteriorSubsystem::UnsubscribeAll()
{
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		auto UnregisterHandle = [&](FOutcomeHandlerHandle& Handle, UOutcomeConditionAsset*& Asset)
		{
			if (Handle.IsValid())
			{
				EventBus->UnregisterHandler(Handle);
				Handle.Invalidate();
			}
			Asset = nullptr;
		};

		UnregisterHandle(SpawnRegisterHandle,   SpawnConditionAsset);
		UnregisterHandle(DespawnRegisterHandle, DespawnConditionAsset);

		UnregisterHandle(PlacementRegisterHandle,   PlacementRegisterConditionAsset);
		UnregisterHandle(PlacementUnregisterHandle, PlacementUnregisterConditionAsset);

		UnregisterHandle(InteractCommandHandle, InteractCommandConditionAsset);
		UnregisterHandle(SetEnabledHandle,      SetEnabledConditionAsset);
		UnregisterHandle(SetRangeHandle,        SetRangeConditionAsset);
		UnregisterHandle(SetTooltipHandle,      SetTooltipConditionAsset);

		UnregisterHandle(FloorStateSaveHandle,    FloorStateSaveConditionAsset);
		UnregisterHandle(FloorStateRestoreHandle, FloorStateRestoreConditionAsset);
	}

	RegisteredItems.Empty();
	RegistrationListeners.Empty();
	// Не трогаем FloorStateSnapshots (это хранилище снимков)
}

void UInteriorSubsystem::BuildAssetIndex()
{
	// Use AssetRegistry to find all InteriorSet assets and build friendly index
	TArray<FAssetData> Ads;
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("InteriorSetAsset")), Ads, true);

	for (const FAssetData& AD : Ads)
	{
		UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
		if (!IS)
		{
			const FSoftObjectPath SoftPath = AD.ToSoftObjectPath();
			IS = Cast<UInteriorSetAsset>(SoftPath.TryLoad());
		}
		if (!IS) continue;

		const FString SoftPathStr = AD.ToSoftObjectPath().ToString();

		for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
		{
			UFloorAsset* Floor = Ref.LoadSynchronous();
			if (!Floor) continue;
			const FString Key = FString::Printf(TEXT("%s:floor_%d"), *SoftPathStr, Floor->FloorIndex);
			FInteriorFloorKey K(IS->InteriorSetID, Floor->FloorID);
			FriendlyFloorIndex.Add(Key, K);
		}
	}
}

bool UInteriorSubsystem::TryResolveFloorKeyByAssetName(const FString& AssetName, int32 FloorIndex, FInteriorFloorKey& OutKey) const
{
	if (AssetName.IsEmpty()) return false;

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("InteriorSetAsset")), AssetDataList, true);

	TArray<FAssetData> Matches;
	for (const FAssetData& AD : AssetDataList)
	{
		if (AD.AssetName.ToString().Equals(AssetName, ESearchCase::IgnoreCase))
		{
			Matches.Add(AD);
		}
	}

	if (Matches.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryResolveFloorKeyByAssetName: no InteriorSet asset named '%s'"), *AssetName);
		return false;
	}

	if (Matches.Num() > 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryResolveFloorKeyByAssetName: multiple InteriorSet assets named '%s', using first match."), *AssetName);
	}

	const FAssetData& AD = Matches[0];
	UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
	if (!IS) IS = Cast<UInteriorSetAsset>(AD.ToSoftObjectPath().TryLoad());
	if (!IS)
	{
		UE_LOG(LogTemp, Warning, TEXT("TryResolveFloorKeyByAssetName: failed to load InteriorSet '%s'"), *AD.ToSoftObjectPath().ToString());
		return false;
	}

	for (const TSoftObjectPtr<UFloorAsset>& FloorRef : IS->Floors)
	{
		if (UFloorAsset* Floor = FloorRef.LoadSynchronous())
		{
			if (Floor->FloorIndex == FloorIndex)
			{
				OutKey = FInteriorFloorKey(IS->InteriorSetID, Floor->FloorID);
				return true;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("TryResolveFloorKeyByAssetName: InteriorSet '%s' does not contain floor index %d"), *AssetName, FloorIndex);
	return false;
}
