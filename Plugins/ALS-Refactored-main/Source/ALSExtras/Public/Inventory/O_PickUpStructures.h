#pragma once

#include "CoreMinimal.h"
#include "O_PickUpStructures.generated.h"

USTRUCT(BlueprintType)
struct FPickUpSaveData
{
	GENERATED_BODY()

	
	UPROPERTY()
	TMap<FGuid, FTransform> PlacedFlyers;
};