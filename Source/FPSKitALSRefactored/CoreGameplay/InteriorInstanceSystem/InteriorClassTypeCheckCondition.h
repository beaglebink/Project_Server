#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "FloorPopulationTypes.h"
#include "InteriorCheckTypes.h"
#include "InteriorClassTypeCheckCondition.generated.h"

/**
 * Base class for checks by class and EFloorActorType.
 * Counts objects on the current level with UFloorAssignmentComponent,
 * matching the specified class and/or type.
 *
 * Базовый класс для проверок по классу и типу EFloorActorType.
 * Подсчитывает объекты на текущем уровне с UFloorAssignmentComponent,
 * соответствующие указанному классу и/или типу.
 */
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorClassTypeCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Filter by actor class (if not set, all classes are checked)
    // Фильтр по классу актора (если не задан, проверяются все классы)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    TSubclassOf<AActor> ActorClass;

    // Filter by EFloorActorType (if None – all types)
    // Фильтр по типу EFloorActorType (если None – все типы)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    EFloorActorType ActorType = EFloorActorType::LightItem;

    // Quantity comparison operator
    // Оператор сравнения количества
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    // Expected quantity
    // Ожидаемое количество
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    int32 ExpectedValue = 0;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    // Returns the number of objects matching the filters
    // Возвращает количество объектов, удовлетворяющих фильтрам
    int32 GetFilteredObjectCount(UWorld* World) const;

    bool bApproved = false;
    bool bCompleted = false;
};

/**
 * Count of objects of the specified class and type (existing).
 *
 * Подсчёт количества объектов указанного класса и типа (существующих).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeCountCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()
};

/**
 * Count of objects of the specified class and type that have been destroyed.
 *
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
 * Count of objects of the specified class and type that remain (not destroyed).
 *
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

/**
 * Count of spawned objects of the specified class and type.
 *
 * Подсчёт количества заспавненных объектов указанного класса и типа.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeSpawnedCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()
public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of spawned objects of the specified class and type that have been destroyed.
 *
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
 * Count of original (initially existing on the level) objects of the specified class and type that have been destroyed.
 *
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

/**
 * Count of spawned objects of the specified class and type that remain (not destroyed by spawn-kill).
 *
 * Подсчёт количества заспавненных объектов указанного класса и типа, которые остались (не уничтожены как спавненные).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeRemainingSpawnedCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of original (initially existing on the level) objects of the specified class and type that remain (not destroyed).
 *
 * Подсчёт количества оригинальных (изначально существовавших на уровне) объектов указанного класса и типа, которые остались (не уничтожены).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorClassTypeRemainingOriginalCondition : public UInteriorClassTypeCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};