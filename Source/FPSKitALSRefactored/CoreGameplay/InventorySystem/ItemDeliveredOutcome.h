#pragma once
#include "OutcomeEventBase.h"
#include "ItemDeliveredOutcome.generated.h"

USTRUCT(BlueprintType)
struct FItemDeliveredOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid ActorId;

    UPROPERTY(BlueprintReadWrite)
    FGuid ObjectId;

    UPROPERTY(BlueprintReadWrite)
    FGuid ReceiverId;
};
