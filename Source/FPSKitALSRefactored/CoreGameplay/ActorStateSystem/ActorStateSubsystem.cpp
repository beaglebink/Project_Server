#include "ActorStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ActorStatePayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// √арантируем что EventBusSubsystem будет инициализирован ƒќ ActorStateSubsystem
	Collection.InitializeDependency(UEventBusSubsystem::StaticClass());

	Super::Initialize(Collection);

	CachedEventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	if (!CachedEventBus.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("ActorStateSubsystem: CachedEventBus is NULL after Initialize - all subscriptions skipped!"));
		return;
	}

	// Subscribe interaction registration early (before actors call BeginPlay)
	SubscribeRegistration();

	// Subscribe interact command handler €вно Ч не зависит от статуса SubscribeRegistration
	SubscribeInteractCommand();

	// Subscribe set enabled €вно Ч не зависит от статуса SubscribeRegistration
	SubscribeSetEnabled();

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

	// InteractRegistered Ч Actor category
	RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Actor;
	RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->FilterRow.ActorType = EOutcomeActor::InteractRegistered;
	RegisteredConditionAsset->FilterRow.ActorComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->CompileCondition();

	// InteractUnregistered Ч Actor category
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

	// SubscribeInteractCommand() теперь вызываетс€ из Initialize() €вно Ч убрано отсюда
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
	UnsubscribeInteractCommand();
	UnsubscribeSetEnabled();
	UnsubscribeRegistration();
}

void UActorStateSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	// ƒелегирование в базовый класс
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UActorStateSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	// ƒелегирование в базовый класс
	FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
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

void UActorStateSubsystem::SubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("ActorStateSubsystem::SubscribeInteractCommand - CachedEventBus is NULL, skipping! (this=%p)"), this);
		return;
	}
	if (InteractCommandHandle.IsValid()) return;

	InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
	InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Actor;
	InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	InteractCommandConditionAsset->CompileCondition();

	if (InteractCommandConditionAsset->GetCondition().IsValid())
	{
		InteractCommandHandle = CachedEventBus->RegisterHandler(
			InteractCommandConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleInteractCommand)
		);
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Subscribed to InteractCommand (handle=%u) on EventBus %p (this=%p)"),
			InteractCommandHandle.GetId(), CachedEventBus.Get(), this);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ActorStateSubsystem: InteractCommand condition compiled to INVALID - handler NOT registered!"));
	}
}

void UActorStateSubsystem::UnsubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(InteractCommandHandle);
		InteractCommandHandle.Invalidate();
	}
	InteractCommandConditionAsset = nullptr;
}

void UActorStateSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem::HandleInteractCommand called (this=%p)"), this);

	if (const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: InteractCommand for ItemId=%s, RegisteredItems.Num=%d"),
			*Id.ToString(), RegisteredItems.Num());

		if (const FActorInteractItemRecord* Rec = RegisteredItems.Find(Id))
		{
			AActor* Owner = Rec->OwnerActor.Get();
			if (Owner)
			{
				ExecuteInteractCommandOnOwner(Id, Owner, P->Picker);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: InteractCommand - Owner is null for ItemId=%s"), *Id.ToString());
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: InteractCommand - ItemId=%s NOT found in RegisteredItems"), *Id.ToString());
		}
	}
}

void UActorStateSubsystem::SubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || SetEnabledHandle.IsValid()) return;

	SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
	SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Actor;
	SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->FilterRow.ActorType = EOutcomeActor::InteractSetEnabled;
	SetEnabledConditionAsset->FilterRow.ActorComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->CompileCondition();

	if (SetEnabledConditionAsset->GetCondition().IsValid())
	{
		SetEnabledHandle = CachedEventBus->RegisterHandler(
			SetEnabledConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleSetEnabled)
		);
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Subscribed to SetEnabled (handle=%u)"), SetEnabledHandle.GetId());
	}
}

void UActorStateSubsystem::UnsubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || !SetEnabledHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetEnabledHandle);
	SetEnabledHandle.Invalidate();
	SetEnabledConditionAsset = nullptr;
}

void UActorStateSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload))
	{
		if (const FActorInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
		}
	}
}

