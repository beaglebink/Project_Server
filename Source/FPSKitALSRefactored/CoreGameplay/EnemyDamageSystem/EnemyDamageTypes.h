#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyDamageTypes.generated.h"

UENUM(BlueprintType)
enum class EDefenseLayerType : uint8
{
    // Persistent Resistance
    // Постоянное сопротивление
    Resistance  UMETA(DisplayName = "Persistent Resistance"),

    // Depletable Reserve
    // Истощаемый резерв
    Reserve     UMETA(DisplayName = "Depletable Reserve")
};

UENUM(BlueprintType)
enum class EStaggerInputType : uint8
{
    // Final Health Damage (Use all defence)
    // Итоговый урон по здоровью (с учётом всей защиты)
    UseFinalHealthDamage   UMETA(DisplayName = "Final Health Damage (Use all defence)"),

    // Total Damage (Without defense reserve)
    // Общий урон (без учёта защиты и резервов)
    UseTotalDamageDealt    UMETA(DisplayName = "Total Damage (Without defense reserve)"),

    // Attack Strength (Before defense)
    // Сила атаки (до применения защиты)
    UseAttackStrength      UMETA(DisplayName = "Attack Strength (Before defense)")
};

USTRUCT(BlueprintType)
struct FRegenerationParams
{
    GENERATED_BODY()

    // Regeneration rate per second
    // Скорость регенерации в секунду
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenRatePerSecond = 0.0f;

    // Delay after taking damage before regeneration starts
    // Задержка после получения урона перед началом регенерации
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenDelayAfterDamage = 0.0f;

    // Whether regeneration is interrupted on damage
    // Прерывается ли регенерация при получении урона
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration")
    bool bInterruptOnDamage = false;

    // Whether regeneration continues while staggered
    // Продолжается ли регенерация во время стаггера
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration")
    bool bRegenWhileStaggered = false;
};

USTRUCT(BlueprintType)
struct FHealthZoneDefinition
{
    GENERATED_BODY()

    // Zone tag (e.g., "High", "Medium", "Low")
    // Тег зоны (например, "High", "Medium", "Low")
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Zone")
    FName ZoneTag;

    // Upper health bound for this zone (exclusive lower bound, inclusive upper bound)
    // Верхняя граница здоровья для этой зоны (нижняя граница исключена, верхняя включена)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Zone", meta = (ClampMin = "0.0"))
    float UpperBound = 100.0f;

    // Lower health bound for this zone
    // Нижняя граница здоровья для этой зоны
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Zone", meta = (ClampMin = "0.0"))
    float LowerBound = 0.0f;
};

USTRUCT(BlueprintType)
struct FDefenseLayer
{
    GENERATED_BODY()

    // Type of defense layer: Resistance or Reserve
    // Тип слоя защиты: Resistance или Reserve
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense")
    EDefenseLayerType LayerType = EDefenseLayerType::Resistance;

    // Resistance multiplier (0.0 = no resistance, 1.0 = full resistance)
    // Множитель сопротивления (0.0 = нет сопротивления, 1.0 = полное сопротивление)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Resistance", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0"))
    float ResistanceMultiplier = 1.0f;

    // Maximum reserve capacity
    // Максимальная ёмкость резерва
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve", EditConditionHides))
    float MaxReserve = 0.0f;

    // Initial reserve value at spawn
    // Начальное значение резерва при спавне
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve", EditConditionHides))
    float InitialReserve = 0.0f;

    // Regeneration parameters for reserve layers
    // Параметры регенерации для слоёв-резервов
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve", EditConditionHides))
    FRegenerationParams ReserveRegen;

    // Layer tag for identification and events
    // Тег слоя для идентификации и событий
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense")
    FName LayerTag;
};

USTRUCT(BlueprintType)
struct FHitZoneDefinition
{
    GENERATED_BODY()

    // Zone name (e.g., "Head", "Chest", "Legs")
    // Название зоны (например, "Head", "Chest", "Legs")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    FName ZoneName;

    // Array of bone names
    // Массив имён костей
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    TArray<FName> BoneNames;

    // Array of component names
    // Массив имён компонентов
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    TArray<FName> ComponentNames;

    // Damage multiplier for this hit zone
    // Множитель урона для этой зоны попадания
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    float DamageMultiplier = 1.0f;
};