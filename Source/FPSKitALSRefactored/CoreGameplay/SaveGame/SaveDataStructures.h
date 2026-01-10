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

    // Новый флаг: для особых компонентов трансформ не восстанавливаем
    UPROPERTY()
    bool bSkipTransformRestore = false;
};

USTRUCT()
struct FActorSaveData
{
    GENERATED_BODY()

    UPROPERTY()
    FString UniqueID;

    UPROPERTY()
    FString ClassName;

    // Трансформ сохраняем только для обычных акторов
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

    // Новый флаг: если true — трансформ и скорости не восстанавливаем
    UPROPERTY()
    bool bSkipTransformRestore = false;

    UPROPERTY()
    FVector LinearVelocity;

    UPROPERTY()
    FVector AngularVelocity;

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