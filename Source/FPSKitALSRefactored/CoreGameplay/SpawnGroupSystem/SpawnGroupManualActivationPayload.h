#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupLocationPayloadBase.h"
#include "SpawnGroupManualActivationPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupManualActivationPayload : public USpawnGroupLocationPayloadBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    USpawnGroupManualActivationPayload* Setup(const FGuid& InGroupId)
    {
        GroupId = InGroupId;
        return this;
    }
};