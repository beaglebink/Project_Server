#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SaveGameHelper.h"
#include "ThisSaveGame.generated.h"

USTRUCT()
struct FActorSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FString UniqueID;

    UPROPERTY()
    FString ClassName;

    UPROPERTY()
    FTransform Transform;

    UPROPERTY()
    FString SerializedData;

    UPROPERTY()
    TArray<FComponentSaveData> SavedComponents;
};

USTRUCT()
struct FSubsystemSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FString SubsystemName;

    UPROPERTY()
    FString SerializedData;
};

UCLASS()
class UThisSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TArray<FActorSaveData> SavedActors;

    UPROPERTY()
    TArray<FSubsystemSaveData> SavedSubsystems;
};