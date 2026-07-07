// InteriorObjectCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "InteriorCheckTypes.h"
#include "InteriorObjectCheckCondition.generated.h"

/**
 * Проверка существования конкретного объекта на текущем уровне.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorObjectExistsCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Ссылка на актор, существование которого проверяется
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior|Object")
    TSoftObjectPtr<AActor> ActorRef;

    // Ожидаемое состояние: true – объект должен существовать, false – не должен существовать
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior|Object")
    bool bShouldExist = true;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    bool bApproved = false;
    bool bCompleted = false;
};

/**
 * Проверка, был ли конкретный объект уничтожен (удалён из мира).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorObjectDestroyedCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Ссылка на актор, уничтожение которого проверяется
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior|Object")
    TSoftObjectPtr<AActor> ActorRef;

    // Ожидаемое состояние: true – объект должен быть уничтожен, false – не должен быть уничтожен
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior|Object")
    bool bShouldBeDestroyed = true;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    bool bApproved = false;
    bool bCompleted = false;
};