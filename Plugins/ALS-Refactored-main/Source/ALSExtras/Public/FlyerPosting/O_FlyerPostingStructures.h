#pragma once

#include "CoreMinimal.h"
#include "ChoreSystem/ChoreEnums.h"
#include "O_FlyerPostingStructures.generated.h"

USTRUCT(BlueprintType)
struct FFlyerPostingSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	EChoreStatus ChoreStatus;

	UPROPERTY()
	TMap<FGuid, FTransform> PlacedFlyers;
};