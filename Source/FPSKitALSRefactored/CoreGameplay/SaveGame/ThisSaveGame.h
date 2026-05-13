#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveDataStructures.h"
#include "ThisSaveGame.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UThisSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
	FString MapName;

    UPROPERTY()
	FTransform PlayerTransform;

    UPROPERTY()
    TArray<FActorSaveData> SavedActors;

    UPROPERTY()
    TArray<FSubsystemSaveData> SavedSubsystems;
};