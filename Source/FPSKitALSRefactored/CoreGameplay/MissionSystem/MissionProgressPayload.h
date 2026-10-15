// MissionProgressPayload.h
#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionAsset.h"
#include "MissionProgressPayload.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UMissionProgressPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionProgress")
    FName MissionName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionProgress")
    UMissionAsset* Asset;

    // Индекс достигнутого шага
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MissionProgress")
    int32 StepIndex = -1;

    UFUNCTION(BlueprintCallable, Category = "MissionProgress")
    UMissionProgressPayload* Setup(/*FName InMissionName, */UMissionAsset* InAsset, int32 InStepIndex = -1)
    {
        if (InAsset)
        {
            MissionName = InAsset->GetMissionId();
        }
        StepIndex = InStepIndex;
        return this;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
    FName GetMissionName() const { return MissionName; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
    int32 GetStepIndex() const { return StepIndex; }

    virtual void OnBeforePublish(const FCommandSpec* Spec, const UPayloadBatchEntry* Entry) override;
};