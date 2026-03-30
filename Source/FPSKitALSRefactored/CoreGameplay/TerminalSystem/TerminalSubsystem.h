#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "TerminalSubsystem.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UTerminalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Called when world begins play (Вызывается когда мир начинает игру)
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
	// Handler for TerminalTaskCompleted events (Обработчик для TerminalTaskCompleted событий)
	UFUNCTION()
	void OnTerminalTaskCompleted(const FOutcomeEventBase& Outcome);
};
