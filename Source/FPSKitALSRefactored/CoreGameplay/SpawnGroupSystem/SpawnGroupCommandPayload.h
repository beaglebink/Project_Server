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
    UPROPERTY(BlueprintReadWrite, Category = "SpawnGroup")
    ESpawnGroupResolutionReason Reason = ESpawnGroupResolutionReason::None;

    // NEW: если true — после ухода с этажа спавнер сохраняется как Inactive,
    // и при возврате враги не восстанавливаются.
    // Дизайнер включает галочку в шаге сценария, когда нужен "одноразовый" спавн.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bPreventAutoRespawn = false;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    USpawnGroupCommandPayload* Setup(const FGuid& InGroupId,
        ESpawnGroupResolutionReason InReason = ESpawnGroupResolutionReason::None)
    {
        GroupId = InGroupId;
        Reason = InReason;
        return this;
    }
};