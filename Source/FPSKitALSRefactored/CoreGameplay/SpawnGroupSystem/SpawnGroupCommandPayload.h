#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "SpawnGroupTypes.h"
#include "SpawnGroupCommandPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup")
    FSpawnGroupId GroupId;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup")
    ESpawnGroupResolutionReason Reason = ESpawnGroupResolutionReason::None;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    USpawnGroupCommandPayload* Setup(const FSpawnGroupId& InGroupId, ESpawnGroupResolutionReason InReason = ESpawnGroupResolutionReason::None)
    {
        GroupId = InGroupId;
        Reason = InReason;
        return this;
    }
};