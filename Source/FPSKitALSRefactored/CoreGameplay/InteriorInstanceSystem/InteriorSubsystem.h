#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "InteriorSubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Called when world begins play (Вызывается когда мир начинает игру)
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	// Handler for GhostCleared events in interior (Обработчик для GhostCleared событий в интерьере)
	UFUNCTION()
	void OnGhostClearedWithInterior(const FOutcomeEventBase& Outcome);

	// Handler for ItemAcquired events in interior (Обработчик для ItemAcquired событий в интерьере)
	UFUNCTION()
	void OnItemAcquiredWithObject(const FOutcomeEventBase& Outcome);
};
