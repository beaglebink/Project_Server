#pragma once
#include "OutcomeEventBase.h"
#include "MissionCompletedOutcome.generated.h"

// Outcome structure for mission completion (Структура результата для завершения миссии)
USTRUCT(BlueprintType)
struct FMissionCompletedOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Mission identifier (Идентификатор миссии)
	UPROPERTY(BlueprintReadWrite)
	FGuid MissionId;

	// Global fact key for world state tracking (Глобальный ключ факта для отслеживания состояния мира)
	UPROPERTY(BlueprintReadWrite)
	FString GlobalFactKey;
};
