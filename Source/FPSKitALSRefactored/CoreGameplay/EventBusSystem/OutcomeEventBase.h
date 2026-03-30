#pragma once
#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomeEventBase.generated.h"

// Base outcome event structure with common properties
USTRUCT(BlueprintType)
struct FOutcomeEventBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	EOutcomeType OutcomeType = EOutcomeType::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeMission OutcomeMission = EOutcomeMission::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeActor OutcomeActor = EOutcomeActor::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeObject OutcomeObject = EOutcomeObject::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeInterior OutcomeInterior = EOutcomeInterior::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeSpawnGroup OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;
};
