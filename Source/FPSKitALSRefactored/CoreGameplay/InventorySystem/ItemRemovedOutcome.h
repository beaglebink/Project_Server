#pragma once
#include "OutcomeEventBase.h"
#include "ItemRemovedOutcome.generated.h"

USTRUCT(BlueprintType)
struct FItemRemovedOutcome : public FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGuid ActorId;

    UPROPERTY(BlueprintReadWrite)
    FGuid ObjectId;
};
