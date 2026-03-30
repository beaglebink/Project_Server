#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "InventorySubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// Initialize subsystem and register event handlers (Инициализация подсистемы и регистрация обработчиков событий)
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	// Handler for ItemAcquired events (Обработчик для ItemAcquired событий)
	UFUNCTION()
	void OnItemAcquired(const FOutcomeEventBase& Outcome);
};
