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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION()
    void HandleOutcome(const FOutcomeEventBase& Outcome);
};
