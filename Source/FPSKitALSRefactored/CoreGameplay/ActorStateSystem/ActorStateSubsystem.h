#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "ActorStateSubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UActorStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Initialize subsystem and register event handlers (Инициализация подсистемы и регистрация обработчиков событий)
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	// Handler for DialogueStarted events (Обработчик для DialogueStarted событий)
	UFUNCTION()
	void OnDialogueStarted(const FOutcomeEventBase& Outcome);

	// Single handler for (MissionCompleted OR GhostCleared) AND SpawnGroup != Default (Один обработчик для (MissionCompleted ИЛИ GhostCleared) И не Default спауна)
	UFUNCTION()
	void OnMissionOrGhostWithSpawn(const FOutcomeEventBase& Outcome);
};
