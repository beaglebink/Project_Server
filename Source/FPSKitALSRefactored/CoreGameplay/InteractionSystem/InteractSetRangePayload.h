#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteractSetRangePayload.generated.h"

/**
 * Payload for the command to change interaction range via EventBus.
 * (Payload для команды изменения радиуса интеракции через EventBus.)
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteractSetRangePayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "InteractSetRange")
	FGuid ItemId;

	UPROPERTY(BlueprintReadWrite, Category = "InteractSetRange")
	float NewRange = 0.f;

	UFUNCTION(BlueprintCallable, Category = "InteractSetRange")
	UInteractSetRangePayload* Setup(const FGuid& InItemId, float InRange)
	{
		ItemId = InItemId;
		NewRange = InRange;
		return this;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetRange")
	FGuid GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetRange")
	float GetNewRange() const { return NewRange; }
};