#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Subscribe all handlers on startup
	// (Подписываем все обработчики при старте)
	SubscribeGhostCleared();
	SubscribeMissionProgress();
}

void UMissionSubsystem::Deinitialize()
{
	UnsubscribeAll();
	Super::Deinitialize();
}

void UMissionSubsystem::SubscribeGhostCleared()
{
	// Skip if already subscribed or no condition assigned
	// (Пропускаем если уже подписан или нет Asset условия)
	if (GhostClearedHandle.IsValid() || !GhostClearedCondition) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		GhostClearedHandle = EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleGhostCleared)
		);
	}
}

void UMissionSubsystem::SubscribeMissionProgress()
{
	if (MissionProgressHandle.IsValid() || !MissionProgressCondition) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		MissionProgressHandle = EventBus->RegisterHandler(
			MissionProgressCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleMissionProgress)
		);
	}
}

void UMissionSubsystem::UnsubscribeGhostCleared()
{
	if (!GhostClearedHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(GhostClearedHandle);
	}
}

void UMissionSubsystem::UnsubscribeMissionProgress()
{
	if (!MissionProgressHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		EventBus->UnregisterHandler(MissionProgressHandle);
	}
}

void UMissionSubsystem::UnsubscribeAll()
{
	UnsubscribeGhostCleared();
	UnsubscribeMissionProgress();
}

void UMissionSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	const FGhostClearedOutcome& Ghost = static_cast<const FGhostClearedOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost %s cleared"), *Ghost.GhostType);

	OnGhostCleared.Broadcast(Ghost);
}

void UMissionSubsystem::HandleMissionProgress(const FOutcomeEventBase& Outcome)
{
	const FMissionProgressOutcome& Progress = static_cast<const FMissionProgressOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission %s step %d"),
		*Progress.MissionName, Progress.StepIndex);

	OnMissionProgress.Broadcast(Progress);
}

