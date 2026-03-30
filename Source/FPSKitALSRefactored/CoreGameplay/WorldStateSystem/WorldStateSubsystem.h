#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "WorldStateSubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UWorldStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Initialize subsystem and register event handlers (Инициализация подсистемы и регистрация обработчиков событий)
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	// Handler for MissionCompleted events (Обработчик для MissionCompleted событий)
	UFUNCTION()
	void OnMissionCompleted(const FOutcomeEventBase& Outcome);
};
