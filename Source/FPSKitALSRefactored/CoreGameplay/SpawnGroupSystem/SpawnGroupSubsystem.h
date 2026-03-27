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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION()
    void HandleOutcome(const FOutcomeEventBase& Outcome);
};
