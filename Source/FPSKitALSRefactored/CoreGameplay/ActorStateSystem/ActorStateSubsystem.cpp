#include "ActorStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ActorStatePayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	CachedEventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	// Subscribe interaction registration early (before actors call BeginPlay)
	SubscribeRegistration();

	// Lazy subscribe when condition already assigned
	if (DialogueStartedCondition) SubscribeDialogueStarted();
	if (MissionOrGhostCondition) SubscribeMissionOrGhost();
}

void UActorStateSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UActorStateSubsystem::SetDialogueStartedCondition(UOutcomeConditionAsset* NewCondition)
{
	if (DialogueStartedCondition == NewCondition) return;
	DialogueStartedCondition = NewCondition;
	if (DialogueStartedHandle.IsValid()) UnsubscribeDialogueStarted();
	SubscribeDialogueStarted();
}

void UActorStateSubsystem::SetMissionOrGhostCondition(UOutcomeConditionAsset* NewCondition)
{
	if (MissionOrGhostCondition == NewCondition) return;
	MissionOrGhostCondition = NewCondition;
	if (MissionOrGhostHandle.IsValid()) UnsubscribeMissionOrGhost();
	SubscribeMissionOrGhost();
}

void UActorStateSubsystem::SubscribeDialogueStarted()
{
	if (DialogueStartedHandle.IsValid() || !DialogueStartedCondition) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		DialogueStartedHandle = EventBus->RegisterHandler(
			DialogueStartedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleDialogueStarted)
		);
	}
}

void UActorStateSubsystem::SubscribeMissionOrGhost()
{
	if (MissionOrGhostHandle.IsValid() || !MissionOrGhostCondition) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		MissionOrGhostHandle = EventBus->RegisterHandler(
			MissionOrGhostCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleMissionOrGhost)
		);
	}
}

void UActorStateSubsystem::UnsubscribeDialogueStarted()
{
	if (!DialogueStartedHandle.IsValid()) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get()) EventBus->UnregisterHandler(DialogueStartedHandle);
	DialogueStartedHandle.Invalidate();
}

void UActorStateSubsystem::UnsubscribeMissionOrGhost()
{
	if (!MissionOrGhostHandle.IsValid()) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get()) EventBus->UnregisterHandler(MissionOrGhostHandle);
	MissionOrGhostHandle.Invalidate();
}

void UActorStateSubsystem::SubscribeRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (RegisteredRegisterHandle.IsValid() || UnregisteredRegisterHandle.IsValid()) return;

	// InteractRegistered — Actor category
	RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Actor;
	RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->FilterRow.ActorType = EOutcomeActor::InteractRegistered;
	RegisteredConditionAsset->FilterRow.ActorComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->CompileCondition();

	// InteractUnregistered — Actor category
	UnregisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	UnregisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	UnregisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Actor;
	UnregisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	UnregisteredConditionAsset->FilterRow.ActorType = EOutcomeActor::InteractUnregistered;
	UnregisteredConditionAsset->FilterRow.ActorComparison = EConditionComparison::Equals;
	UnregisteredConditionAsset->CompileCondition();

	if (RegisteredConditionAsset->GetCondition().IsValid())
	{
		RegisteredRegisterHandle = CachedEventBus->RegisterHandler(
			RegisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Subscribed to InteractRegistered (handle=%u)"), RegisteredRegisterHandle.GetId());
	}

	if (UnregisteredConditionAsset->GetCondition().IsValid())
	{
		UnregisteredRegisterHandle = CachedEventBus->RegisterHandler(
			UnregisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Subscribed to InteractUnregistered (handle=%u)"), UnregisteredRegisterHandle.GetId());
	}
}

void UActorStateSubsystem::UnsubscribeRegistration()
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

void UActorStateSubsystem::UnsubscribeAll()
{
	UnsubscribeDialogueStarted();
	UnsubscribeMissionOrGhost();
	UnsubscribeRegistration();
}

void UActorStateSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	if (!Listener) return;
	TArray<TWeakObjectPtr<UInteractiveItemComponent>>& Arr = RegistrationListeners.FindOrAdd(ItemId);
	for (auto& W : Arr)
	{
		if (W.Get() == Listener) return;
	}
	Arr.Add(Listener);
}

void UActorStateSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
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
		if (Arr->Num() == 0) RegistrationListeners.Remove(ItemId);
	}
}

void UActorStateSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	if (UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		AActor* Owner  = P->GetOwnerActor();
		FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

		if (Outcome.OutcomeActor == EOutcomeActor::InteractRegistered)
		{
			FActorInteractItemRecord R;
			R.ItemId = Id;
			R.OwnerActor = Owner;
			R.InteractionRange = P->InteractionRange;
			RegisteredItems.Add(Id, R);

			UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Registered interactive item owned by '%s' (range=%.1f)"),
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
		else if (Outcome.OutcomeActor == EOutcomeActor::InteractUnregistered)
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

			UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Unregistered interactive item owned by '%s'"), *NameToLog);

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

void UActorStateSubsystem::HandleDialogueStarted(const FOutcomeEventBase& Outcome)
{
	if (DialogueStartedCondition)
	{
		auto Query = DialogueStartedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}
	if (UActorStatePayload* P = Cast<UActorStatePayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Dialogue started - %s"), *P->StateChangeType);
	}
	OnDialogueStarted.Broadcast(Outcome);
}

void UActorStateSubsystem::HandleMissionOrGhost(const FOutcomeEventBase& Outcome)
{
	if (MissionOrGhostCondition)
	{
		auto Query = MissionOrGhostCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}
	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Mission or Ghost - SpawnGroup: %d"),
		static_cast<int32>(Outcome.OutcomeSpawnGroup));
	OnMissionOrGhostWithSpawn.Broadcast(Outcome);
}

