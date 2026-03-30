#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "MissionSubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Initialize subsystem and register event handlers (Инициализация подсистемы и регистрация обработчиков событий)
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	// Handler for GhostCleared with conditions (Обработчик для GhostCleared с условиями)
	UFUNCTION()
	void OnGhostClearedWithConditions(const FOutcomeEventBase& Outcome);

	// Handler for ItemAcquired with mission (Обработчик для ItemAcquired с миссией)
	UFUNCTION()
	void OnItemAcquiredWithMission(const FOutcomeEventBase& Outcome);
};
