// DamageProcessing.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "EnemyDamageTypes.h"
#include "DamageProcessing.generated.h"

class UEnemyDamageConfig;
class UDamageProcessingEffect;

// Контекст для модификации специальными эффектами (передаётся в UDamageProcessingEffect)
USTRUCT(BlueprintType)
struct FDamageProcessingContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float IncomingDamage = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    UEnemyDamageConfig* Config = nullptr;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> Reserves;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ResistanceMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ReserveDamageMultipliers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<bool> BypassReserve;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<bool> IgnoreLayer;

    UPROPERTY(BlueprintReadWrite)
    float FinalHealthDamageMultiplier = 1.0f;

    UPROPERTY(BlueprintReadWrite)
    float StaggerChanceModifier = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bForceStagger = false;
};

// Структура для передачи модификаторов защиты (используется в ApplyDefenseModifiers)
USTRUCT(BlueprintType)
struct FDefenseModifiers
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ResistanceMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ReserveDamageMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<bool> BypassReserve;

    UPROPERTY(BlueprintReadWrite)
    TArray<bool> IgnoreLayer;
};

// Структура для пост-защитных эффектов (используется в PostDefenseProcessing)
USTRUCT(BlueprintType)
struct FPostDefenseResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float FinalHealthDamage = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float StaggerChanceModifier = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bForceStagger = false;
};

// Информация об атаке, передаваемая при попадании
USTRUCT(BlueprintType)
struct FAttackDamageInfo
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseDamage = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* DamageSource = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AController* Instigator = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float GunConditionModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ClothingModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FHitResult HitResult;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid AttackID;

    // Специальные эффекты атаки (DataAsset)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDamageProcessingEffect> OptionalEnemyEffects = nullptr;
};

// Результат обработки урона
USTRUCT(BlueprintType)
struct FDamageResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    float IncomingDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float HitZoneMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly)
    float DamageAfterZone = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> LayerAbsorbedDamage;

    UPROPERTY(BlueprintReadOnly)
    float FinalHealthDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float NewHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> NewReserves;

    UPROPERTY(BlueprintReadOnly)
    FName CurrentHealthZoneTag;

    UPROPERTY(BlueprintReadOnly)
    bool bHealthZoneChanged = false;

    UPROPERTY(BlueprintReadOnly)
    bool bZeroDamage = false;

    UPROPERTY(BlueprintReadOnly)
    bool bStaggerTriggered = false;

    UPROPERTY(BlueprintReadOnly)
    bool bKilled = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<bool> ReserveDepleted;

    UPROPERTY(BlueprintReadOnly)
    float TotalDamageDealt = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float AttackStrength = 0.0f;
};