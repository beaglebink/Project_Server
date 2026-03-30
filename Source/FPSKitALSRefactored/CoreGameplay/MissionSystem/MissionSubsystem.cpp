#include "MissionSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus) return;

	if (GhostClearedCondition)
	{
		EventBus->RegisterHandler(
			GhostClearedCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleGhostCleared)
		);
	}

	if (MissionProgressCondition)
	{
		EventBus->RegisterHandler(
			MissionProgressCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UMissionSubsystem::HandleMissionProgress)
		);
	}
}

void UMissionSubsystem::HandleGhostCleared(const FOutcomeEventBase& Outcome)
{
	const FGhostClearedOutcome& Ghost = static_cast<const FGhostClearedOutcome&>(Outcome);

	// C++ logic here (C++ логика здесь)
	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost %s cleared"), *Ghost.GhostType);

	// Broadcast typed struct directly - Blueprint gets FGhostClearedOutcome
	// (Рассылаем типизированную структуру напрямую - Blueprint получает FGhostClearedOutcome)
	OnGhostCleared.Broadcast(Ghost);
}

void UMissionSubsystem::HandleMissionProgress(const FOutcomeEventBase& Outcome)
{
	const FMissionProgressOutcome& Progress = static_cast<const FMissionProgressOutcome&>(Outcome);

	// C++ logic here (C++ логика здесь)
	UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Mission %s step %d"),
		*Progress.MissionName, Progress.StepIndex);

	// Broadcast typed struct directly - Blueprint gets FMissionProgressOutcome
	// (Рассылаем типизированную структуру напрямую - Blueprint получает FMissionProgressOutcome)
	OnMissionProgress.Broadcast(Progress);
}

