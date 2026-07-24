#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "SpawnGroupTypes.h"
#include "SpawnGroupRegistrationPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupRegistrationPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup")
    FGuid SpawnerId;

    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup")
    FGuid GroupId;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    USpawnGroupRegistrationPayload* Setup(const FGuid& InSpawnerId)
    {
        SpawnerId = InSpawnerId;
        //GroupId = InGroupId;
        return this;
    }
};