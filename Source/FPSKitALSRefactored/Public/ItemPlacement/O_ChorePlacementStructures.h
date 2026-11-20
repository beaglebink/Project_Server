#pragma once

#include "CoreMinimal.h"
#include "ChoreSystem/ChoreEnums.h"
#include "O_ChorePlacementStructures.generated.h"

USTRUCT(BlueprintType)
struct FSaveItemData
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid ItemId;

	UPROPERTY()
	FGuid AttachingDropZoneId;

	UPROPERTY()
	FTransform ItemTransform;

	UPROPERTY()
	uint8 bIsHeld : 1{false};

	UPROPERTY()
	uint8 bIsAttached : 1{false};

	UPROPERTY()
	uint8 bHasSimulatedPhysics : 1{false};

	// Breakable actor
	UPROPERTY()
	uint8 bIsBroken : 1{false};
};

USTRUCT(BlueprintType)
struct FSaveChoreData
{
	GENERATED_BODY()

	UPROPERTY()
	EChoreStatus ChoreStatus;

	UPROPERTY()
	uint8 bWasStarted : 1;

	UPROPERTY()
	TArray<FSaveItemData> SaveItemData;
};