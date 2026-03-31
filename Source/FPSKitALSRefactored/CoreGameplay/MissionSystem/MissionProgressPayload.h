#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionProgressPayload.generated.h"

// Payload for MissionProgress events
// (Payload для событий прогресса миссий)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UMissionProgressPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "MissionProgress")
	FGuid MissionId;

	UPROPERTY(BlueprintReadWrite, Category = "MissionProgress")
	FString MissionName;

	UPROPERTY(BlueprintReadWrite, Category = "MissionProgress")
	int32 StepIndex = 0;

	// Fill all fields in one call - use after CreatePayload
	// C++:  Payload->Setup(MissionId, MissionName, StepIndex)
	// BP:   Setup node after CreatePayload → Cast To UMissionProgressPayload
	// (Заполняет все поля одним вызовом - использовать после CreatePayload)
	UFUNCTION(BlueprintCallable, Category = "MissionProgress")
	UMissionProgressPayload* Setup(
		const FGuid&   InMissionId,
		const FString& InMissionName,
		int32          InStepIndex)
	{
		MissionId   = InMissionId;
		MissionName = InMissionName;
		StepIndex   = InStepIndex;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
	FGuid GetMissionId() const { return MissionId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
	FString GetMissionName() const { return MissionName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
	int32 GetStepIndex() const { return StepIndex; }
};