#include "SpawnGroupSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "GhostClearedPayload.h"

void USpawnGroupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	if (GhostClearedCondition)
	{
		SubscribeGhostCleared();
	}
}

void USpawnGroupSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void USpawnGroupSubsystem::SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition)
{
	if (GhostClearedCondition == NewCondition) return;
	GhostClearedCondition = NewCondition;
	if (GhostClearedHandle.IsValid())
	{
		UnsubscribeGhostCleared();
	}
	SubscribeGhostCleared();
}

void USpawnGroupSubsystem::SubscribeGhostCleared()
{
	if (GhostClearedHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSubsystem: Already subscribed to GhostCleared"));
		return;
	}

	if (!GhostClearedCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnGroupSubsystem: GhostClearedCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &USpawnGroupSubsystem::HandleGhostCleared)
		);

		UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Subscribed to GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
}

void USpawnGroupSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
		UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Unsubscribed GhostCleared (handle=%u)"), GhostClearedHandle.GetId());
	}
	GhostClearedHandle.Invalidate();
}

void USpawnGroupSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
}

void USpawnGroupSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	if (GhostClearedCondition)
	{
		auto Query = GhostClearedCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("SpawnGroupSubsystem: Incoming outcome does not satisfy GhostClearedCondition -> ignoring"));
			return;
		}
	}

	if (UGhostClearedPayload* P = Cast<UGhostClearedPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Ghost %s cleared"), *P->GhostType);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("SpawnGroupSubsystem: HandleGhostCleared called with no payload or unexpected payload type"));
	}

	OnGhostCleared.Broadcast(Outcome);
}
