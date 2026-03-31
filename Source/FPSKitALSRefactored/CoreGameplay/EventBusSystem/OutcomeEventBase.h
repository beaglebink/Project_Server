#pragma once
#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomeEventBase.generated.h"

// Base outcome event structure with all category fields
// (Базовая структура события со всеми полями категорий)
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

	// Terminal events - separate from Object to keep categories clean
	// (Терминальные события - отдельно от Object для чистоты категорий)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeTerminal OutcomeTerminal = EOutcomeTerminal::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeInterior OutcomeInterior = EOutcomeInterior::Default;

	UPROPERTY(BlueprintReadWrite)
	EOutcomeSpawnGroup OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;

	// World state changes triggered by this event
	// (Изменения мирового состояния вызванные этим событием)
	UPROPERTY(BlueprintReadWrite)
	EWorldState WorldState = EWorldState::Default;
};
