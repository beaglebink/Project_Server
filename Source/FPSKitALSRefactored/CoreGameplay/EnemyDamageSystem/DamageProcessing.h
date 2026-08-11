#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "EnemyDamageTypes.h"
#include "DamageProcessing.generated.h"

class UEnemyDamageConfig;

// Контекст для модификации специальными эффектами (передаётся в UDamageProcessingEffect)
USTRUCT(BlueprintType)
struct FDamageProcessingContext
{
    GENERATED_BODY()

    // Указатель на входящий урон (может быть изменён эффектом)
    float* IncomingDamage = nullptr;

    // Конфиг защиты (для чтения/модификации)
    UEnemyDamageConfig* Config = nullptr;

    // Текущие резервы (массив, соответствующий слоям)
    TArray<float>* Reserves = nullptr;
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

    // Специальные эффекты атаки
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<class UDamageProcessingEffect> OptionalEnemyEffects = nullptr;
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

    // Поглощённый урон по слоям (индексы соответствуют DefenseLayers конфига)
    UPROPERTY(BlueprintReadOnly)
    TArray<float> LayerAbsorbedDamage;

    UPROPERTY(BlueprintReadOnly)
    float FinalHealthDamage = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float NewHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> NewReserves; // новые значения резервов

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

    // Дополнительно: был ли истощён какой-либо резерв
    UPROPERTY(BlueprintReadOnly)
    TArray<bool> ReserveDepleted;

    // Реальный урон, пошедший в здоровье и резервы (сумма списанного)
    UPROPERTY(BlueprintReadOnly)
    float TotalDamageDealt = 0.0f;

    // Сила атаки после модификаторов оружия, одежды и зоны попадания (до защиты)
    UPROPERTY(BlueprintReadOnly)
    float AttackStrength = 0.0f;
};