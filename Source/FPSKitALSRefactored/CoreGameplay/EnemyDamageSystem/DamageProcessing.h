// DamageProcessing.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "EnemyDamageTypes.h"
#include "DamageProcessing.generated.h"

class UEnemyDamageConfig;
class UDamageProcessingEffect;

USTRUCT(BlueprintType)
struct FDamageProcessingContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float IncomingDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    UEnemyDamageConfig* Config = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> Reserves;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ResistanceMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ReserveDamageMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<bool> IgnoreLayer;

    UPROPERTY(BlueprintReadWrite)
    float FinalHealthDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float Health = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FName PreviousHealthZone;

    UPROPERTY(BlueprintReadOnly)
    FName CurrentHealthZone;

    //UPROPERTY(BlueprintReadWrite)
    float StaggerChanceModifier = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    AActor* Owner = nullptr;

    //UPROPERTY(BlueprintReadOnly)
    //AActor* Weapon = nullptr;

    //UPROPERTY(BlueprintReadWrite)
    float ForceStaggerCooldown = 2.0f;

    UPROPERTY(BlueprintReadWrite)
	bool IsStagger = false;
};

// Выходные структуры (используются в эффектах)
USTRUCT(BlueprintType)
struct FPreDefenseOutput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float NewIncomingDamage = 0.0f;
};

USTRUCT(BlueprintType)
struct FDefenseModifiers
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ResistanceMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<float> ReserveDamageMultipliers;

    UPROPERTY(BlueprintReadWrite)
    TArray<bool> IgnoreLayer;
};

USTRUCT(BlueprintType)
struct FDefenseOutput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FDefenseModifiers NewModifiers;
};

USTRUCT(BlueprintType)
struct FPostDefenseOutput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float NewFinalHealthDamage = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    float NewStaggerChanceModifier = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    bool bNewForceStagger = false;

    UPROPERTY(BlueprintReadWrite)
    float bNewForceStaggerCooldown = 2.0f;
};

// Информация об атаке
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UDamageProcessingEffect> OptionalEnemyEffects = nullptr;
};

// Результат урона
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
    float MaxHealth = 0.0f;

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

    //UPROPERTY(BlueprintReadOnly)
    //float AttackStrength = 0.0f;
};