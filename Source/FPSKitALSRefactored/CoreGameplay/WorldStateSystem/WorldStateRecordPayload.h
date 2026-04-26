#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "../WorldStateSystem/WorldStateTypes.h"
#include "WorldStateRecordPayload.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UWorldStateRecordPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "WorldState")
	FWorldStateRecord Record;

	UFUNCTION(BlueprintCallable, Category = "WorldState")
	UWorldStateRecordPayload* Setup(const FWorldStateRecord& InRecord)
	{
		Record = InRecord;
		return this;
	}
};