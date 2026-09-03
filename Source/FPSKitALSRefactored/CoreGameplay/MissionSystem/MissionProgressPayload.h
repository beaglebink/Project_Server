// MissionProgressPayload.h
#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionProgressPayload.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UMissionProgressPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "MissionProgress")
    FName MissionName;

    // Новое поле: индекс достигнутого шага
    UPROPERTY(BlueprintReadWrite, Category = "MissionProgress")
    int32 StepIndex = -1;

    UFUNCTION(BlueprintCallable, Category = "MissionProgress")
    UMissionProgressPayload* Setup(FName InMissionName, int32 InStepIndex = -1)
    {
        MissionName = InMissionName;
        StepIndex = InStepIndex;
        return this;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
    FName GetMissionName() const { return MissionName; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
    int32 GetStepIndex() const { return StepIndex; }
};