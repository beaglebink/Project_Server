#pragma once
#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomeEventBase.generated.h"

// Base outcome event structure with common properties (Базовая структура события результата с общими свойствами)
USTRUCT(BlueprintType)
struct FOutcomeEventBase
{
	GENERATED_BODY()

	// Event type (Тип события)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeType OutcomeType = EOutcomeType::Default;

	// Mission category (Категория миссии)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeMission OutcomeMission = EOutcomeMission::Default;

	// Actor category (Категория актера)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeActor OutcomeActor = EOutcomeActor::Default;

	// Object category (Категория объекта)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeObject OutcomeObject = EOutcomeObject::Default;

	// Interior category (Категория интерьера)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeInterior OutcomeInterior = EOutcomeInterior::Default;

	// Spawn group category (Категория группы спауна)
	UPROPERTY(BlueprintReadWrite)
	EOutcomeSpawnGroup OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;
};
