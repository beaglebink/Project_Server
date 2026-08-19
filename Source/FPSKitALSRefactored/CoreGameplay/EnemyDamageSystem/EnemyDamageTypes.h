// EnemyDamageTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyDamageTypes.generated.h"

UENUM(BlueprintType)
enum class EDefenseLayerType : uint8
{
    Resistance  UMETA(DisplayName = "Persistent Resistance"),
    Reserve     UMETA(DisplayName = "Depletable Reserve")
};

UENUM(BlueprintType)
enum class EStaggerInputType : uint8
{
    UseFinalHealthDamage   UMETA(DisplayName = "Final Health Damage"),
    UseTotalDamageDealt    UMETA(DisplayName = "Total Damage (Health + Reserves)"),
    UseAttackStrength      UMETA(DisplayName = "Attack Strength (before defense)")
};

USTRUCT(BlueprintType)
struct FRegenerationParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenRatePerSecond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration", meta = (ClampMin = "0.0"))
    float RegenDelayAfterDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration")
    bool bInterruptOnDamage = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Regeneration")
    bool bRegenWhileStaggered = false;
};

USTRUCT(BlueprintType)
struct FHealthZoneDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Zone")
    FName ZoneTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Zone", meta = (ClampMin = "0.0"))
    float UpperBound = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Zone", meta = (ClampMin = "0.0"))
    float LowerBound = 0.0f;
};

USTRUCT(BlueprintType)
struct FDefenseLayer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense")
    EDefenseLayerType LayerType = EDefenseLayerType::Resistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Resistance", EditConditionHides , ClampMin = "0.0", ClampMax = "1.0"))
    float ResistanceMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve", EditConditionHides))
    float MaxReserve = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve", EditConditionHides))
    float InitialReserve = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve", EditConditionHides))
    FRegenerationParams ReserveRegen;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Defense")
    FName LayerTag;
};

USTRUCT(BlueprintType)
struct FHitZoneDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    FName ZoneName;

    // Массив имён костей
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    TArray<FName> BoneNames;

    // Массив имён компонентов
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    TArray<FName> ComponentNames;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Zone")
    float DamageMultiplier = 1.0f;
};