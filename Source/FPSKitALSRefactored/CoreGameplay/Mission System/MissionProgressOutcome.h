#pragma once
#include "OutcomeEventBase.h"
#include "MissionProgressOutcome.generated.h"

USTRUCT(BlueprintType)
struct FMissionProgressOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid MissionId;

    UPROPERTY(BlueprintReadWrite)
    int32 StepIndex;
};
