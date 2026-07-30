// SpawnGroupAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpawnGroupTypes.h"
#include "../MissionSystem/MissionEnvelopeTypes.h"
#include <AlsCharacterExample.h>
#include "SpawnGroupAsset.generated.h"

USTRUCT(BlueprintType)
struct FSpawnTypeCount
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AAlsCharacter> ActorClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count;
};

// Definition of group composition: either an explicit list or a reference to a pool.
// Определение состава группы: либо явный список, либо ссылка на пул.
USTRUCT(BlueprintType)
struct FSpawnGroupComposition
{
    GENERATED_BODY()

    // If true – use the pool (then List is ignored)
    // Если true – используем пул (тогда List игнорируется)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUsePool = false;

    // Pool identifier (e.g., "Bandits.Melee")
    // Идентификатор пула (например, "Bandits.Melee")
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUsePool"))
    //FName PoolId;

    // Explicit list of actor classes with parameters (for custom groups)
    // Явный список классов акторов с параметрами (для авторских групп)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUsePool", EditConditionHides))
    TArray<TSubclassOf<AAlsCharacter>> ActorClasses;

    // Number of spawns (if pool, it may be a range)
    // Количество спавнов (если пул, то может быть диапазон)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUsePool", EditConditionHides))
    int32 Count = 1;

    // Additional parameters: weights, difficulty levels, etc. (optional)
    // Доп. параметры: веса, уровни сложности и т.д. (опционально)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUsePool", EditConditionHides))
    TArray<FSpawnTypeCount> ActorsPool;
};

// Description of a spawn point (anchor, zone, relative coordinates)
// Описание точки спавна (якорь, зона, относительные координаты)
USTRUCT(BlueprintType)
struct FSpawnPoint
{
    GENERATED_BODY()

    // Anchor GUID (preferred)
    // GUID якоря (предпочтительно)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid AnchorId;

    // Or a soft reference to a zone/room
    // Либо мягкая ссылка на зону/комнату
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ZoneTag;

    // Local transform relative to the anchor/zone
    // Локальный трансформ относительно якоря/зоны
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform RelativeTransform;
};

UCLASS(BlueprintType)
class USpawnGroupAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Display name (for debugging)
    // Отображаемое имя (для дебага)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Identity")
    FText DisplayName;

    // Scope: floor (default) or building
    // Область действия: этаж (по умолчанию) или здание
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Scope")
    bool bBuildingWide = false;

    // If !bBuildingWide, bind to a specific floor (optional, otherwise inherited from context)
    // Если !bBuildingWide, то привязка к конкретному этажу (опционально, иначе наследуется от контекста)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Scope", meta = (EditCondition = "!bBuildingWide"))
    TSoftObjectPtr<class UFloorAsset> TargetFloor;

    // Group composition
    // Состав группы
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Composition")
    FSpawnGroupComposition Composition;

    // Spawn points (if not specified, use default anchors)
    // Точки спавна (если не указаны, использовать якоря по умолчанию)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Placement")
    TArray<FSpawnPoint> SpawnPoints;

    // Can the group be "partially cleared"
    // Может ли группа быть "частично зачищена"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Policy")
    bool bAllowPartialClear = true;

    // Override – condition for "cleared" (e.g., capture a specific NPC)
    // Переопределение – условие "очищено" (например, захватить определённого NPC)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Completion")
    UClass* CompletionConditionClass; // inherits from UGroupCompletionCondition // наследник UGroupCompletionCondition

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("SpawnGroup", GetFName());
    }
};