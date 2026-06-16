#pragma once

#include "CoreMinimal.h"
#include "CubixonCoreData.generated.h"

USTRUCT(BlueprintType)
struct FCubixonFileData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame, BlueprintReadWrite)
	FText FileName;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TSubclassOf<USceneComponent> ComponentClass = nullptr;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TArray<uint8> SerializedData;
};
