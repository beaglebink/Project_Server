#pragma once
#include "OutcomeEventBase.h"
#include "GhostClearedOutcome.generated.h"

USTRUCT(BlueprintType)
struct FGhostClearedOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid InteriorSetId;

    UPROPERTY(BlueprintReadWrite)
    FGuid SpawnGroupId;
};
