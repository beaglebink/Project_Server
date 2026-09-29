#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "WorldStateRecordRemovePayload.generated.h"

/**
 * Payload for the command to remove a world state record via EventBus.
 * Removal is performed by FactId (the string identifier of the world fact).
 *
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