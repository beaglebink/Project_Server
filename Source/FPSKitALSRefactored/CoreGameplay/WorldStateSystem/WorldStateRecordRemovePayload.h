#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "WorldStateRecordRemovePayload.generated.h"

/**
 * Payload для команды удаления записи о состоянии мира через EventBus.
 * Удаление выполняется по FactId (строковому идентификатору мирового факта).
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UWorldStateRecordRemovePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName FactId;

    UFUNCTION(BlueprintCallable, Category = "WorldState")
    UWorldStateRecordRemovePayload* Setup(FName InFactId)
    {
        FactId = InFactId;
        return this;
    }
};