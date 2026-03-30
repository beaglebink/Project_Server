#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "SpawnGroupSubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API USpawnGroupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Called when world begins play (Вызывается когда мир начинает игру)
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	// Handler for GhostCleared events with spawn conditions (Обработчик для GhostCleared событий с условиями спауна)
	UFUNCTION()
	void OnGhostClearedWithSpawn(const FOutcomeEventBase& Outcome);
};
