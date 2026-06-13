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

// Определение состава группы: либо явный список, либо ссылка на пул.
USTRUCT(BlueprintType)
struct FSpawnGroupComposition
{
    GENERATED_BODY()

    // Если true – используем пул (тогда List игнорируется)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUsePool = false;

    // Идентификатор пула (например, "Bandits.Melee")
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUsePool"))
    //FName PoolId;

    // Явный список классов акторов с параметрами (для авторских групп)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUsePool", EditConditionHides))
    TArray<TSubclassOf<AAlsCharacter>> ActorClasses;

    // Количество спавнов (если пул, то может быть диапазон)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "!bUsePool", EditConditionHides))
    int32 Count = 1;

    // Доп. параметры: веса, уровни сложности и т.д. (опционально)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bUsePool", EditConditionHides))
    TArray<FSpawnTypeCount> ActorsPool;
};

// Описание точки спавна (якорь, зона, относительные координаты)
USTRUCT(BlueprintType)
struct FSpawnPoint
{
    GENERATED_BODY()

    // GUID якоря (предпочтительно)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid AnchorId;

    // Либо мягкая ссылка на зону/комнату
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ZoneTag;

    // Локальный трансформ относительно якоря/зоны
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FTransform RelativeTransform;
};

UCLASS(BlueprintType)
class USpawnGroupAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Стабильный идентификатор группы (задаётся дизайнером)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Identity")
    FGuid GroupId;

    // Отображаемое имя (для дебага)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Identity")
    FText DisplayName;

    // Область действия: этаж (по умолчанию) или здание
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Scope")
    bool bBuildingWide = false;

    // Если !bBuildingWide, то привязка к конкретному этажу (опционально, иначе наследуется от контекста)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Scope", meta = (EditCondition = "!bBuildingWide"))
    TSoftObjectPtr<class UFloorAsset> TargetFloor;

    // Состав группы
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Composition")
    FSpawnGroupComposition Composition;

    // Точки спавна (если не указаны, использовать якоря по умолчанию)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Placement")
    TArray<FSpawnPoint> SpawnPoints;

    // Политика сброса по умолчанию (может быть переопределена миссией через канал SpawnGroups)
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Policy")
    //EChannelPolicy DefaultResetPolicy = EChannelPolicy::Reset;

    // Может ли группа быть "частично зачищена"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Policy")
    bool bAllowPartialClear = true;

    // Переопределение – условие "очищено" (например, захватить определённого NPC)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SpawnGroup|Completion")
    UClass* CompletionConditionClass; // наследник UGroupCompletionCondition

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("SpawnGroup", GetFName());
    }

#if WITH_EDITOR
    virtual void PostInitProperties() override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};