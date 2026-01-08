#pragma once

#include "CoreMinimal.h"
#include "SaveDataStructures.generated.h"

USTRUCT()
struct FComponentSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FString ComponentName;

    UPROPERTY()
    FString SerializedData;
};

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

    UPROPERTY()
    FString AttachParentID;

    UPROPERTY()
    FName AttachSocketName;
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