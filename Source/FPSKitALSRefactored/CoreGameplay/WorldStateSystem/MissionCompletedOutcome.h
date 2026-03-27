#pragma once
#include "OutcomeEventBase.h"
#include "MissionCompletedOutcome.generated.h"

USTRUCT(BlueprintType)
struct FMissionCompletedOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid MissionId;

    UPROPERTY(BlueprintReadWrite)
    FString GlobalFactKey;
};
