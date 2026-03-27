#pragma once
#include "OutcomeEventBase.h"
#include "InteriorTransitionOutcome.generated.h"

USTRUCT(BlueprintType)
struct FInteriorTransitionOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid InteriorSetId;

    UPROPERTY(BlueprintReadWrite)
    int32 FloorIndex;
};
