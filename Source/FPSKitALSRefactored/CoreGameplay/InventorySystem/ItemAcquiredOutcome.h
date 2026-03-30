#pragma once
#include "OutcomeEventBase.h"
#include "ItemAcquiredOutcome.generated.h"

// Outcome structure when item is acquired (Структура результата при получении предмета)
USTRUCT(BlueprintType)
struct FItemAcquiredOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Actor identifier who acquired the item (Идентификатор актера, который получил предмет)
	UPROPERTY(BlueprintReadWrite)
	FGuid ActorId;

	// Object/Item identifier (Идентификатор объекта/предмета)
	UPROPERTY(BlueprintReadWrite)
	FGuid ObjectId;
};
