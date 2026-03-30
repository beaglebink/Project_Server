#pragma once
#include "OutcomeEventBase.h"
#include "MissionProgressOutcome.generated.h"

// Outcome structure for mission progress updates (Структура результата для обновления прогресса миссии)
USTRUCT(BlueprintType)
struct FMissionProgressOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Mission identifier (Идентификатор миссии)
	UPROPERTY(BlueprintReadWrite)
	FGuid MissionId;

	// Mission name for logging and debugging (Имя миссии для логирования и отладки)
	UPROPERTY(BlueprintReadWrite)
	FString MissionName;

	// Current step index in mission progression (Текущий индекс шага в прогрессии миссии)
	UPROPERTY(BlueprintReadWrite)
	int32 StepIndex;
};
