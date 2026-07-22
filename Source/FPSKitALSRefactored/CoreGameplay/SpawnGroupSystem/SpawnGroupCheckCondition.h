// SpawnGroupCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "SpawnGroupTypes.h"
#include "GameplayTagContainer.h"
#include "SpawnGroupCheckCondition.generated.h"

class ASpawnGroupSpawner;
class USpawnGroupSubsystem;

/**
 * Базовый класс для всех условий проверки спавн-групп.
 * Содержит общую логику получения спавнера и подсистемы.
 * Не содержит оператора сравнения – используется для статусных проверок.
 */
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API USpawnGroupConditionBase : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Ссылка на конкретный спавнер. Если не задан – проверка по текущему этажу (агрегированно).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup", meta = (DisplayName = "Spawner Actor"))
    TSoftObjectPtr<ASpawnGroupSpawner> SpawnerReference;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    // Возвращает указатель на спавнер, если он загружен и валиден
    ASpawnGroupSpawner* GetSpawner() const;

    // Возвращает эффективный GroupId (из спавнера или пустой для этажа)
    FGuid GetEffectiveGroupId() const;

    USpawnGroupSubsystem* GetSpawnGroupSubsystem() const;

    bool bApproved = false;
    bool bCompleted = false;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

// ============================================================================
// Базовый класс для количественных проверок (с оператором сравнения)
// ============================================================================
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API USpawnGroupCheckCondition : public USpawnGroupConditionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    int32 ExpectedValue = 0;

    virtual FString GetDescription() const override;

protected:
    // Сравнение количества с ожидаемым значением по оператору
    bool EvaluateCompare(int32 ActualValue) const;
};

// ============================================================================
// 1. Проверка статуса группы (без оператора и ожидаемого значения)
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupStatusCondition : public USpawnGroupConditionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup", meta = (DisplayName = "Status"))
    ESpawnGroupStatus DesiredStatus = ESpawnGroupStatus::Active;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 2–10. Количественные проверки (используют Operator и ExpectedValue)
// ============================================================================

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupTotalCountCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveCountCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledCountCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledByTypeCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TSubclassOf<AActor> ActorClass;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveByTypeCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TSubclassOf<AActor> ActorClass;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledByTextTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    FName TextTag;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveByTextTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    FName TextTag;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledByGameplayTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    FGameplayTag GameplayTag;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveByGameplayTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    FGameplayTag GameplayTag;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};