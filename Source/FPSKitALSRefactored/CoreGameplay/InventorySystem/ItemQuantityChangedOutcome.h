#pragma once
#include "OutcomeEventBase.h"
#include "ItemQuantityChangedOutcome.generated.h"

USTRUCT(BlueprintType)
struct FItemQuantityChangedOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid ActorId;

    UPROPERTY(BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY(BlueprintReadWrite)
    int32 NewQuantity;
};
