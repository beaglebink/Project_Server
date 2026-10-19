#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupLocationPayloadBase.h"
#include "SpawnGroupTypes.h"
#include "SpawnGroupCommandPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupCommandPayload : public USpawnGroupLocationPayloadBase
{
    GENERATED_BODY()

public:
    // GroupId находится в базовом классе.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    ESpawnGroupResolutionReason Reason = ESpawnGroupResolutionReason::None;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    USpawnGroupCommandPayload* Setup(const FGuid& InGroupId,
        ESpawnGroupResolutionReason InReason = ESpawnGroupResolutionReason::None)
    {
        GroupId = InGroupId;
        Reason = InReason;
        return this;
    }
};