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

    UPROPERTY()
    FString AttachParentName;   // имя родительского компонента

    UPROPERTY()
    FName AttachSocketName;     // сокет

    UPROPERTY()
    FTransform RelativeTransform; // локальный трансформ
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
    FString AttachParentID;             // имя родительского актора

    UPROPERTY()
    FString AttachParentComponentPath;  // путь к компоненту‑родителю

    UPROPERTY()
    FName AttachSocketName;             // сокет
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