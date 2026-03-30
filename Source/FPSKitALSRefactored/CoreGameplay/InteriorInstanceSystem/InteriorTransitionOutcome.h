#pragma once
#include "CoreMinimal.h"
#include "OutcomeEventBase.h"
#include "InteriorTransitionOutcome.generated.h"

// Outcome structure for interior transitions (Структура результата для переходов интерьера)
USTRUCT(BlueprintType)
struct FInteriorTransitionOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Interior set identifier (Идентификатор набора интерьера)
	UPROPERTY(BlueprintReadWrite)
	FGuid InteriorSetId;

	// Floor index for multi-floor interiors (Индекс этажа для многоэтажных интерьеров)
	UPROPERTY(BlueprintReadWrite)
	int32 FloorIndex = 0;
};
