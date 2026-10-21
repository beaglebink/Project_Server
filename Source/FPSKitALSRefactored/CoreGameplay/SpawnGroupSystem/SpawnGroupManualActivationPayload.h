#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupLocationPayloadBase.h"
#include "SpawnGroupManualActivationPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupManualActivationPayload : public USpawnGroupLocationPayloadBase
{
    GENERATED_BODY()

public:
    // true  — спавнер работает только по внешним командам (триггер / BP).
    //         Активация из шагов сценария игнорируется.
    // false — обычный режим.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bManualActivationOnly = true;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    USpawnGroupManualActivationPayload* Setup(const FGuid& InGroupId, bool bInManualActivationOnly = true)
    {
        GroupId = InGroupId;
        bManualActivationOnly = bInManualActivationOnly;
        return this;
    }
};