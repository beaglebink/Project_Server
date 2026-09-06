#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "WorldStateRecordRemovePayload.generated.h"

/**
 * Payload для команды удаления записи о состоянии мира через EventBus.
 * Используется вместе с UWorldStateSubsystem::WorldStateRecordRemoveCondition.
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UWorldStateRecordRemovePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FGuid ItemId;

    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName ChangeKey;

    UFUNCTION(BlueprintCallable, Category = "WorldState")
    UWorldStateRecordRemovePayload* Setup(const FGuid& InItemId, FName InChangeKey)
    {
        ItemId = InItemId;
        ChangeKey = InChangeKey;
        return this;
    }
};