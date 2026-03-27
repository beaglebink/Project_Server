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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION()
    void HandleOutcome(const FOutcomeEventBase& Outcome);
};
