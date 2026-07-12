#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "InteriorCheckTypes.h"
#include "InteriorObjectCheckCondition.generated.h"

/**
 * Check for existence of a specific object on the current level.
 *
 * Проверка существования конкретного объекта на текущем уровне.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorObjectExistsCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Reference to the actor whose existence is being checked
    // Ссылка на актор, существование которого проверяется
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    TSoftObjectPtr<AActor> ActorRef;

    // Expected state: true – object must exist, false – must not exist
    // Ожидаемое состояние: true – объект должен существовать, false – не должен существовать
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
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
 * Check whether a specific object has been destroyed (removed from the world).
 *
 * Проверка, был ли конкретный объект уничтожен (удалён из мира).
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorObjectDestroyedCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Reference to the actor whose destruction is being checked
    // Ссылка на актор, уничтожение которого проверяется
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    TSoftObjectPtr<AActor> ActorRef;

    // Expected state: true – object must be destroyed, false – must not be destroyed
    // Ожидаемое состояние: true – объект должен быть уничтожен, false – не должен быть уничтожен
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
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