#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "ResetSpawnerSnapshotPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UResetSpawnerSnapshotPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Snapshot")
    FGuid ItemId;

    UFUNCTION(BlueprintCallable, Category = "Snapshot")
    UResetSpawnerSnapshotPayload* Setup(const FGuid& InItemId)
    {
        ItemId = InItemId;
        return this;
    }
};