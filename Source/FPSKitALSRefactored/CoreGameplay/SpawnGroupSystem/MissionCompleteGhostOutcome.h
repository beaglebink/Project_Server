#pragma once
#include "OutcomeEventBase.h"
#include "MissionCompleteGhostOutcome.generated.h"

// Combined outcome structure for mission completion and ghost clearing (Объединённая структура результата для завершения миссии и очищения привидения)
USTRUCT(BlueprintType)
struct FMissionCompleteGhostOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Mission identifier (Идентификатор миссии)
	UPROPERTY(BlueprintReadWrite)
	FGuid MissionId;

	// Ghost type identifier (Идентификатор типа привидения)
	UPROPERTY(BlueprintReadWrite)
	FString GhostType;

	// Interior set identifier (Идентификатор набора интерьера)
	UPROPERTY(BlueprintReadWrite)
	FGuid InteriorSetId;

	// Spawn group identifier (Идентификатор группы спауна)
	UPROPERTY(BlueprintReadWrite)
	FGuid SpawnGroupId;

	// Additional data field 1 (Дополнительное поле данных 1)
	UPROPERTY(BlueprintReadWrite)
	FString SomeData1;

	// Additional data field 2 (Дополнительное поле данных 2)
	UPROPERTY(BlueprintReadWrite)
	float SomeData2;
};