#pragma once
#include "OutcomeEventBase.h"
#include "ItemAcquiredOutcome.generated.h"

USTRUCT(BlueprintType)
struct FItemAcquiredOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid ActorId;

    UPROPERTY(BlueprintReadWrite)
    FGuid ObjectId;
};
