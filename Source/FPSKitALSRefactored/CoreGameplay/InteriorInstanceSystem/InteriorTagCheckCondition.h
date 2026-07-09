// InteriorTagCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "InteriorCheckTypes.h"
#include "GameplayTagContainer.h"
#include "InteriorTagCheckCondition.generated.h"

/**
 * Базовый класс для проверок по тегам (текстовым или GameplayTag).
 * Подсчитывает объекты на текущем уровне с UFloorAssignmentComponent,
 * имеющие указанный тег.
 */
UCLASS(Abstract, Blueprintable)
class FPSKITALSREFACTORED_API UInteriorTagCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Тип тега
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    ETagType TagType = ETagType::TextTag;

    // Текстовый тег (если TagType == TextTag)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior",
        meta = (EditCondition = "TagType == ETagType::TextTag", EditConditionHides))
    FName TextTag;

    // GameplayTag (если TagType == GameplayTag)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior",
        meta = (EditCondition = "TagType == ETagType::GameplayTag", EditConditionHides))
    FGameplayTag GameplayTag;

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
    // Возвращает количество объектов с тегом на текущем уровне
    int32 GetTaggedObjectCount(UWorld* World) const;

    bool bApproved = false;
    bool bCompleted = false;
};

/**
 * Подсчёт количества объектов с тегом (все существующие).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorTagCountCondition : public UInteriorTagCheckCondition
{
    GENERATED_BODY()
};

/**
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