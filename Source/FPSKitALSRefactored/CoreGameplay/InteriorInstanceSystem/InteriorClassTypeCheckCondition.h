// InteriorClassTypeCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "FloorPopulationTypes.h"
#include "InteriorCheckTypes.h"
#include "InteriorClassTypeCheckCondition.generated.h"

/**
 * Базовый класс для проверок по классу и типу EFloorActorType.
 * Подсчитывает объекты на текущем уровне с UFloorAssignmentComponent,
 * соответствующие указанному классу и/или типу.
 */
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorClassTypeCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Фильтр по классу актора (если не задан, проверяются все классы)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    TSubclassOf<AActor> ActorClass;

    // Фильтр по типу EFloorActorType (если None – все типы)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    EFloorActorType ActorType = EFloorActorType::LightItem;

    // Оператор сравнения количества
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    // Ожидаемое количество
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    int32 ExpectedValue = 0;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    // Возвращает количество объектов, удовлетворяющих фильтрам
    int32 GetFilteredObjectCount(UWorld* World) const;

    bool bApproved = false;
    bool bCompleted = false;
};

/**
 * Подсчёт количества объектов указанного класса и типа (существующих).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeCountCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()
};

/**
 * Подсчёт количества объектов указанного класса и типа, которые были уничтожены.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeDestroyedCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Подсчёт количества объектов указанного класса и типа, которые остались (не уничтожены).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeRemainingCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeSpawnedCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

// InteriorClassTypeCheckCondition.h (дополнение)

/**
 * Подсчёт количества заспавненных объектов указанного класса и типа, которые были уничтожены.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeDestroyedSpawnedCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Подсчёт количества оригинальных (изначально существовавших на уровне) объектов указанного класса и типа, которые были уничтожены.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeDestroyedOriginalCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};