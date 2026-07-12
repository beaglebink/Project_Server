#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "InteriorCheckTypes.h"
#include "GameplayTagContainer.h"
#include "InteriorTagCheckCondition.generated.h"

/**
 * Base class for checks by tags (text or GameplayTag).
 * Counts objects on the current level with UFloorAssignmentComponent,
 * that have the specified tag.
 *
 * Базовый класс для проверок по тегам (текстовым или GameplayTag).
 * Подсчитывает объекты на текущем уровне с UFloorAssignmentComponent,
 * имеющие указанный тег.
 */
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorTagCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Tag type
    // Тип тега
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    ETagType TagType = ETagType::TextTag;

    // Text tag (if TagType == TextTag)
    // Текстовый тег (если TagType == TextTag)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior",
        meta = (EditCondition = "TagType == ETagType::TextTag", EditConditionHides))
    FName TextTag;

    // GameplayTag (if TagType == GameplayTag)
    // GameplayTag (если TagType == GameplayTag)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior",
        meta = (EditCondition = "TagType == ETagType::GameplayTag", EditConditionHides))
    FGameplayTag GameplayTag;

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
    // Returns the number of objects with the tag on the current level
    // Возвращает количество объектов с тегом на текущем уровне
    int32 GetTaggedObjectCount(UWorld* World) const;

    bool bApproved = false;
    bool bCompleted = false;
};

/**
 * Count of objects with the tag (all existing).
 *
 * Подсчёт количества объектов с тегом (все существующие).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagCountCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()
};

/**
 * Count of objects with the tag that have been destroyed (recorded in the Destroyed maps).
 *
 * Подсчёт количества объектов с тегом, которые были уничтожены (записаны в Destroyed карты).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagDestroyedCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of objects with the tag that remain (not destroyed).
 * This is the difference between the total and destroyed.
 *
 * Подсчёт количества объектов с тегом, которые остались (не уничтожены).
 * Это разница между общим количеством и уничтоженными.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagRemainingCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of spawned objects with the tag that are still alive (not destroyed).
 *
 * Подсчёт количества заспавненных объектов с тегом, которые ещё живы (не уничтожены).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagSpawnedCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of spawned objects with the tag that have been destroyed.
 *
 * Подсчёт количества заспавненных объектов с тегом, которые были уничтожены.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagDestroyedSpawnedCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of original (initially existing on the level) objects with the tag that have been destroyed.
 *
 * Подсчёт количества оригинальных (изначально существовавших на уровне) объектов с тегом, которые были уничтожены.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagDestroyedOriginalCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};

/**
 * Count of spawned objects with the tag that remain (not destroyed by spawn-kill).
 *
 * Подсчёт количества заспавненных объектов с тегом, которые остались (не уничтожены как спавненные).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagRemainingSpawnedCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()

public:
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual FString GetDescription() const override;
};