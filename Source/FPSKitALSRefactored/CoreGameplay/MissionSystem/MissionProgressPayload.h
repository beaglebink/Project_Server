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
	FName MissionName;

	// Fill all fields in one call - use after CreatePayload
	// C++:  Payload->Setup(MissionName)
	// BP:   Setup node after CreatePayload → Cast To UMissionProgressPayload
	// (Заполняет все поля одним вызовом - использовать после CreatePayload)
	UFUNCTION(BlueprintCallable, Category = "MissionProgress")
	UMissionProgressPayload* Setup(FName InMissionName)
	{
		MissionName = InMissionName;
		return this;
	}

	// Getters

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionProgress")
	FName GetMissionName() const { return MissionName; }
};