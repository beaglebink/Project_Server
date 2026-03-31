#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteriorTransitionPayload.generated.h"

// Payload for InteriorTransition events
// (Payload для событий переходов интерьера)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorTransitionPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadWrite, Category = "InteriorTransition")
	int32 FloorIndex = 0;

	UFUNCTION(BlueprintCallable, Category = "InteriorTransition")
	UInteriorTransitionPayload* Setup(
		const FGuid& InInteriorSetId,
		int32        InFloorIndex)
	{
		InteriorSetId = InInteriorSetId;
		FloorIndex    = InFloorIndex;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	FGuid GetInteriorSetId() const { return InteriorSetId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorTransition")
	int32 GetFloorIndex() const { return FloorIndex; }
};