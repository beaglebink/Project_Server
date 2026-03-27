#pragma once
#include "OutcomeEventBase.h"
#include "GhostSpawnedOutcome.generated.h"

USTRUCT(BlueprintType)
struct FGhostSpawnedOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid SpawnGroupId;

    UPROPERTY(BlueprintReadWrite)
    int32 GhostCount;
};
