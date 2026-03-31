#include "ActorStateSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ActorStatePayload.h"

void UActorStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Lazy subscribe when condition already assigned
	if (DialogueStartedCondition) SubscribeDialogueStarted();
	if (MissionOrGhostCondition) SubscribeMissionOrGhost();
}

void UActorStateSubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UActorStateSubsystem::SetDialogueStartedCondition(UOutcomeConditionAsset* NewCondition)
{
	if (DialogueStartedCondition == NewCondition) return;
	DialogueStartedCondition = NewCondition;
	if (DialogueStartedHandle.IsValid())
	{
		UnsubscribeDialogueStarted();
	}
	SubscribeDialogueStarted();
}

void UActorStateSubsystem::SetMissionOrGhostCondition(UOutcomeConditionAsset* NewCondition)
{
	if (MissionOrGhostCondition == NewCondition) return;
	MissionOrGhostCondition = NewCondition;
	if (MissionOrGhostHandle.IsValid())
	{
		UnsubscribeMissionOrGhost();
	}
	SubscribeMissionOrGhost();
}

void UActorStateSubsystem::SubscribeDialogueStarted()
{
	if (DialogueStartedHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: Already subscribed to DialogueStarted"));
		return;
	}

	if (!DialogueStartedCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: DialogueStartedCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		DialogueStartedHandle = EventBus->RegisterHandler(
			DialogueStartedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UActorStateSubsystem::HandleDialogueStarted)
		);
	}
}

void UActorStateSubsystem::SubscribeMissionOrGhost()
{
	if (MissionOrGhostHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: Already subscribed to MissionOrGhost"));
		return;
	}

	if (!MissionOrGhostCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActorStateSubsystem: MissionOrGhostCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
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

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(DialogueStartedHandle);
	}
	DialogueStartedHandle.Invalidate();
}

void UActorStateSubsystem::UnsubscribeMissionOrGhost()
{
	if (!MissionOrGhostHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(MissionOrGhostHandle);
	}
	MissionOrGhostHandle.Invalidate();
}

void UActorStateSubsystem::UnsubscribeAll()
{
	UnsubscribeDialogueStarted();
	UnsubscribeMissionOrGhost();
}

void UActorStateSubsystem::HandleDialogueStarted(const FOutcomeEventBase& Outcome)
{
	if (DialogueStartedCondition)
	{
		auto Query = DialogueStartedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("ActorStateSubsystem: Incoming outcome does not satisfy DialogueStartedCondition -> ignoring"));
			return;
		}
	}

	if (UActorStatePayload* P = Cast<UActorStatePayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Dialogue started - %s"), *P->StateChangeType);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("ActorStateSubsystem: HandleDialogueStarted called with no payload or unexpected payload type"));
	}

	OnDialogueStarted.Broadcast(Outcome);
}

void UActorStateSubsystem::HandleMissionOrGhost(const FOutcomeEventBase& Outcome)
{
	if (MissionOrGhostCondition)
	{
		auto Query = MissionOrGhostCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("ActorStateSubsystem: Incoming outcome does not satisfy MissionOrGhostCondition -> ignoring"));
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ActorStateSubsystem: Mission or Ghost - SpawnGroup: %d"),
		static_cast<int32>(Outcome.OutcomeSpawnGroup));

	OnMissionOrGhostWithSpawn.Broadcast(Outcome);
}

