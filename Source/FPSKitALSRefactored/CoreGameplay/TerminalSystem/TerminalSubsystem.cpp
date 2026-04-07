#include "TerminalSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "TerminalTaskPayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../EventBusSystem/OutcomeConditionAsset.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"

void UTerminalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	SubscribeRegistration();

	if (TerminalTaskCondition)
	{
		TerminalTaskCondition->CompileCondition();
		SubscribeTerminalTask();
	}

	SubscribeInteractCommand();
	SubscribeSetEnabled();
	SubscribeSetRange();
	SubscribeSetTooltip();
}

void UTerminalSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UTerminalSubsystem::SetTerminalTaskCondition(UOutcomeConditionAsset* NewCondition)
{
	if (TerminalTaskCondition == NewCondition) return;
	TerminalTaskCondition = NewCondition;

	if (TerminalTaskCondition)
	{
		TerminalTaskCondition->CompileCondition();
	}

	if (TerminalTaskHandle.IsValid())
	{
		UnsubscribeTerminalTask();
	}

	if (TerminalTaskCondition)
	{
		SubscribeTerminalTask();
	}
}

void UTerminalSubsystem::SubscribeTerminalTask()
{
	if (TerminalTaskHandle.IsValid()) return;
	if (!TerminalTaskCondition) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		TerminalTaskHandle = EventBus->RegisterHandler(
			TerminalTaskCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleTerminalTask)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to TerminalTask (handle=%u)"), TerminalTaskHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeTerminalTask()
{
	if (!TerminalTaskHandle.IsValid()) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(TerminalTaskHandle);
	}
	TerminalTaskHandle.Invalidate();
}

void UTerminalSubsystem::SubscribeRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (RegisteredRegisterHandle.IsValid() || UnregisteredRegisterHandle.IsValid()) return;

	// Create runtime condition asset for InteractRegistered (Terminal-specific)
	RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
	RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractRegistered;
	RegisteredConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->CompileCondition();

	// Create runtime condition asset for InteractUnregistered
	UnregisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	UnregisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	UnregisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
	UnregisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	UnregisteredConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractUnregistered;
	UnregisteredConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
	UnregisteredConditionAsset->CompileCondition();

	// Register handlers
	if (RegisteredConditionAsset->GetCondition().IsValid())
	{
		RegisteredRegisterHandle = CachedEventBus->RegisterHandler(
			RegisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to InteractRegistered (handle=%u)"), RegisteredRegisterHandle.GetId());
	}

	if (UnregisteredConditionAsset->GetCondition().IsValid())
	{
		UnregisteredRegisterHandle = CachedEventBus->RegisterHandler(
			UnregisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to InteractUnregistered (handle=%u)"), UnregisteredRegisterHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeRegistration()
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

	RegisteredConditionAsset = nullptr;
	UnregisteredConditionAsset = nullptr;
}

void UTerminalSubsystem::UnsubscribeAll()
{
	UnsubscribeTerminalTask();
	UnsubscribeInteractCommand();
	UnsubscribeSetEnabled();
	UnsubscribeRegistration();
	UnsubscribeSetRange();
	UnsubscribeSetTooltip();
}

void UTerminalSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UTerminalSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
	FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

void UTerminalSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
	// Expect registration payload
	if (UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload))
	{
		AActor* Owner = P->GetOwnerActor();
		FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

		// OutcomeTerminal indicates whether it's registered or unregistered
		if (Outcome.OutcomeTerminal == EOutcomeTerminal::InteractRegistered)
		{
			FTerminalInteractItemRecord R;
			R.ItemId = P->ItemId;
			R.OwnerActor = Owner;
			R.InteractionRange = P->InteractionRange;

			RegisteredItems.Add(P->ItemId, R);

			UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Registered interactive item owned by '%s' (range=%.1f)"),
				*OwnerName, P->InteractionRange);

			// Notify only listeners registered for this ItemId
			if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
			{
				for (auto It = List->CreateIterator(); It; ++It)
				{
					if (UInteractiveItemComponent* Comp = It->Get())
					{
						Comp->OnRegisteredBySubsystem(P);
					}
				}
				// After notifying single-use listeners it's reasonable to remove them (they may want persistent subscriptions — design choice)
				RegistrationListeners.Remove(P->ItemId);
			}

			// Legacy / monitoring broadcast (optional)
			// OnInteractItemRegistered.Broadcast(P);
		}
		else if (Outcome.OutcomeTerminal == EOutcomeTerminal::InteractUnregistered)
		{
			// Try to find owner from registry first
			FString NameToLog = OwnerName;
			if (RegisteredItems.Contains(P->ItemId))
			{
				AActor* RegOwner = RegisteredItems[P->ItemId].OwnerActor.Get();
				if (RegOwner)
				{
					NameToLog = RegOwner->GetName();
				}
				RegisteredItems.Remove(P->ItemId);
			}
			else
			{
				// Not in registry - log provided owner or GUID as fallback
				if (NameToLog.IsEmpty() || NameToLog == "Unknown")
				{
					NameToLog = P->ItemId.ToString();
				}
			}

			UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Unregistered interactive item owned by '%s'"),
				*NameToLog);

			// Notify only listeners registered for this ItemId
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

			// Legacy / monitoring broadcast (optional)
			// OnInteractItemUnregistered.Broadcast(P);
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: HandleInteractRegistration called with unexpected payload"));
	}
}

void UTerminalSubsystem::HandleTerminalTask(const FOutcomeEventBase& Outcome)
{
	// If no condition is configured or not compiled yet - ignore incoming events
	if (!TerminalTaskCondition)
	{
		UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: TerminalTaskCondition not set - ignoring incoming TerminalTask event"));
		return;
	}

	auto Query = TerminalTaskCondition->GetCondition();
	if (!Query.IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: TerminalTaskCondition not compiled - ignoring incoming TerminalTask event"));
		return;
	}

	if (!Query->Evaluate(Outcome))
	{
		UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: Incoming outcome does not satisfy TerminalTaskCondition -> ignoring"));
		return;
	}

	if (UTerminalTaskPayload* P = Cast<UTerminalTaskPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Task %s - Success: %s"),
			*P->TaskId, P->bSuccess ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: HandleTerminalTask called with no payload or unexpected payload type"));
	}

	OnTerminalTaskCompleted.Broadcast(Outcome);
}

void UTerminalSubsystem::SubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid()) return;

	InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
	// Подписка на Terminal-тип команд
	InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
	InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	InteractCommandConditionAsset->CompileCondition();

	if (InteractCommandConditionAsset->GetCondition().IsValid())
	{
		InteractCommandHandle = CachedEventBus->RegisterHandler(
			InteractCommandConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleInteractCommand)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to InteractCommand (handle=%u)"), InteractCommandHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(InteractCommandHandle);
		InteractCommandHandle.Invalidate();
	}
	InteractCommandConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	if (const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		if (const FTerminalInteractItemRecord* Rec = RegisteredItems.Find(Id))
		{
			AActor* Owner = Rec->OwnerActor.Get();
			if (Owner)
			{
				ExecuteInteractCommandOnOwner(Id, Owner, P->Picker);
			}
		}
	}
}

void UTerminalSubsystem::SubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || SetEnabledHandle.IsValid()) return;

	SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
	SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
	SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractSetEnabled;
	SetEnabledConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->CompileCondition();

	if (SetEnabledConditionAsset->GetCondition().IsValid())
	{
		SetEnabledHandle = CachedEventBus->RegisterHandler(
			SetEnabledConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleSetEnabled)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to SetEnabled (handle=%u)"), SetEnabledHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || !SetEnabledHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetEnabledHandle);
	SetEnabledHandle.Invalidate();
	SetEnabledConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload))
	{
		if (const FTerminalInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
		}
	}
}

void UTerminalSubsystem::SubscribeSetRange()
{
	if (!CachedEventBus.IsValid() || SetRangeHandle.IsValid()) return;

	SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
	SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
	SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetRangeConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractSetRange;
	SetRangeConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
	SetRangeConditionAsset->CompileCondition();

	if (SetRangeConditionAsset->GetCondition().IsValid())
	{
		SetRangeHandle = CachedEventBus->RegisterHandler(
			SetRangeConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleSetRange)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to SetRange (handle=%u)"), SetRangeHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeSetRange()
{
	if (!CachedEventBus.IsValid() || !SetRangeHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetRangeHandle);
	SetRangeHandle.Invalidate();
	SetRangeConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload))
	{
		if (const FTerminalInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
		}
	}
}

void UTerminalSubsystem::SubscribeSetTooltip()
{
	if (!CachedEventBus.IsValid() || SetTooltipHandle.IsValid()) return;

	SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
	SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
	SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetTooltipConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractSetTooltip;
	SetTooltipConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
	SetTooltipConditionAsset->CompileCondition();

	if (SetTooltipConditionAsset->GetCondition().IsValid())
	{
		SetTooltipHandle = CachedEventBus->RegisterHandler(
			SetTooltipConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleSetTooltip)
		);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to SetTooltip (handle=%u)"), SetTooltipHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeSetTooltip()
{
	if (!CachedEventBus.IsValid() || !SetTooltipHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetTooltipHandle);
	SetTooltipHandle.Invalidate();
	SetTooltipConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload))
	{
		if (const FTerminalInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
		}
	}
}