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
 * Содержит общую логику получения подсистемы, логирование и т.д.
 * Не содержит ссылок на спавнеры – они добавляются в наследниках.
 */
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API USpawnGroupConditionBase : public UCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    USpawnGroupSubsystem* GetSpawnGroupSubsystem() const;

    bool bApproved = false;
    bool bCompleted = false;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

// ============================================================================
// Базовый класс для количественных проверок (с массивом спавнеров и оператором)
// ============================================================================
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API USpawnGroupCheckCondition : public USpawnGroupConditionBase
{
    GENERATED_BODY()

public:
    // Массив конкретных спавнеров. Если пуст – проверка по текущему этажу (агрегированно).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup", meta = (DisplayName = "Spawner Actors"))
    TArray<TSoftObjectPtr<ASpawnGroupSpawner>> SpawnersReferences;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    int32 ExpectedValue = 0;

    virtual FString GetDescription() const override;

protected:
    // Возвращает массив валидных указателей на спавнеры
    TArray<ASpawnGroupSpawner*> GetSpawners() const;

    // Возвращает массив эффективных GroupId (ItemId) для указанных спавнеров (пустой, если спавнеров нет)
    TArray<FGuid> GetEffectiveGroupIds() const;

    // Сравнение количества с ожидаемым значением по оператору
    bool EvaluateCompare(int32 ActualValue) const;
};

// ============================================================================
// 1. Проверка статуса группы (одиночный спавнер)
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupStatusCondition : public USpawnGroupConditionBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup", meta = (DisplayName = "Spawner Actor"))
    TSoftObjectPtr<ASpawnGroupSpawner> Spawner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup", meta = (DisplayName = "Status"))
    ESpawnGroupStatus DesiredStatus = ESpawnGroupStatus::Active;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 2. Общее количество заспавненных
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupTotalCountCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 3. Количество живых
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveCountCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 4. Количество убитых
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledCountCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 5. Убито по классу
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledByClassCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TSubclassOf<AActor> ActorClass;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 6. Живо по классу
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveByClassCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TSubclassOf<AActor> ActorClass;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 7. Убито по текстовым тегам (массив)
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledByTextTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TArray<FName> TextTags;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 8. Живо по текстовым тегам (массив)
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveByTextTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TArray<FName> TextTags;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 9. Убито по GameplayTag (массив)
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupKilledByGameplayTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TArray<FGameplayTag> GameplayTags;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// ============================================================================
// 10. Живо по GameplayTag (массив)
// ============================================================================
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupAliveByGameplayTagCondition : public USpawnGroupCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TArray<FGameplayTag> GameplayTags;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};