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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION()
    void HandleOutcome(const FOutcomeEventBase& Outcome);
};
