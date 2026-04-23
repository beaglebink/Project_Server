#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "MissionProgressPayload.h"

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

    // Lazy subscribe: only subscribe if condition assigned in editor or earlier
    // (Ленивая подписка: подписываемся только если ассет задан в редакторе или раньше)
	if (GhostClearedCondition)
	{
		SubscribeGhostCleared();
	}
	if (MissionProgressCondition)
	{
		SubscribeMissionProgress();
	}
}

void UMissionSubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UMissionSubsystem::SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition)
{
	if (GhostClearedCondition == NewCondition) return;
	GhostClearedCondition = NewCondition;
	if (GhostClearedHandle.IsValid())
	{
		UnsubscribeGhostCleared();
	}
	SubscribeGhostCleared();
}

void UMissionSubsystem::SetMissionProgressCondition(UOutcomeConditionAsset* NewCondition)
{
	if (MissionProgressCondition == NewCondition) return;
	MissionProgressCondition = NewCondition;
	if (MissionProgressHandle.IsValid())
	{
		UnsubscribeMissionProgress();
	}
	SubscribeMissionProgress();
}

void UMissionSubsystem::SubscribeGhostCleared()
{
	if (GhostClearedHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: Already subscribed to GhostCleared"));
		return;
	}

	if (!GhostClearedCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: GhostClearedCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleGhostCleared)
		);

		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Subscribed to GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
}

void UMissionSubsystem::SubscribeMissionProgress()
{
	if (MissionProgressHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: Already subscribed to MissionProgress"));
		return;
	}

	if (!MissionProgressCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("MissionSubsystem: MissionProgressCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		MissionProgressHandle = EventBus->RegisterHandler(
			MissionProgressCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleMissionProgress)
		);

		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Subscribed to MissionProgress (handle=%u)"), MissionProgressHandle.GetId());
	}
}

void UMissionSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Unsubscribed GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
	GhostClearedHandle.Invalidate();
}

void UMissionSubsystem::UnsubscribeMissionProgress()
{
	if (!MissionProgressHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(MissionProgressHandle);
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Unsubscribed MissionProgress (handle=%u)"), MissionProgressHandle.GetId());
	}
	MissionProgressHandle.Invalidate();
}

void UMissionSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeMissionProgress();
}

void UMissionSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	// Defensive check using compiled query (EventBus already filters, this is extra safety)
	// (Защитная проверка с использованием скомпилированного Query (EventBus уже фильтрует))
	if (GhostClearedCondition)
	{
		auto Query = GhostClearedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: Incoming outcome does not satisfy GhostClearedCondition -> ignoring"));
			return;
		}
	}

	if (UGhostClearedPayload* P = Cast<UGhostClearedPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost %s cleared"), *P->GhostType);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: HandleGhostCleared called with no payload or unexpected payload type"));
	}

	OnGhostCleared.Broadcast(Outcome);
}

void UMissionSubsystem::HandleMissionProgress(const FOutcomeEventBase& Outcome)
{
	if (MissionProgressCondition)
	{
		auto Query = MissionProgressCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: Incoming outcome does not satisfy MissionProgressCondition -> ignoring"));
			return;
		}
	}

	if (UMissionProgressPayload* P = Cast<UMissionProgressPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission %s step %d"),
			*P->MissionName, P->StepIndex);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("MissionSubsystem: HandleMissionProgress called with no payload or unexpected payload type"));
	}

	OnMissionProgress.Broadcast(Outcome);
}

