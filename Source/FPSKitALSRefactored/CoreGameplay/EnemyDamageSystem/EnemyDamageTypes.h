#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h" // для возможного использования
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

    UPROPERTY(EditAnywhere, Category = "Regeneration")
    float RegenRatePerSecond = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Regeneration")
    float RegenDelayAfterDamage = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Regeneration")
    bool bInterruptOnDamage = true;

    UPROPERTY(EditAnywhere, Category = "Regeneration")
    bool bRegenWhileStaggered = false;
};

USTRUCT(BlueprintType)
struct FHealthZoneDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Health Zone")
    FName ZoneTag;

    // Зона действует при Health ∈ (LowerBound, UpperBound]
    UPROPERTY(EditAnywhere, Category = "Health Zone", meta = (ClampMin = "0.0"))
    float UpperBound = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Health Zone", meta = (ClampMin = "0.0"))
    float LowerBound = 0.0f;
};

USTRUCT(BlueprintType)
struct FDefenseLayer
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Defense")
    EDefenseLayerType LayerType = EDefenseLayerType::Resistance;

    // Для Resistance
    UPROPERTY(EditAnywhere, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Resistance", ClampMin = "0.0", ClampMax = "1.0"))
    float ResistanceMultiplier = 1.0f;

    // Для Reserve
    UPROPERTY(EditAnywhere, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve"))
    float MaxReserve = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve"))
    float InitialReserve = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Defense", meta = (EditCondition = "LayerType == EDefenseLayerType::Reserve"))
    FRegenerationParams ReserveRegen;

    UPROPERTY(EditAnywhere, Category = "Defense")
    FName LayerTag;
};

USTRUCT(BlueprintType)
struct FHitZoneDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Hit Zone")
    FName ZoneName;

    UPROPERTY(EditAnywhere, Category = "Hit Zone")
    TArray<FName> BoneNames;

    UPROPERTY(EditAnywhere, Category = "Hit Zone")
    float DamageMultiplier = 1.0f;
};