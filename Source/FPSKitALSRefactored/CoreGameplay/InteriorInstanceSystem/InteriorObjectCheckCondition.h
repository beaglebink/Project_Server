#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "InteriorCheckTypes.h"
#include "InteriorObjectCheckCondition.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorObjectExistsCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<AActor> ActorRef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bShouldExist = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    FGuid InitialItemId;

    // Переопределяем Initialize, чтобы кэшировать информацию об акторе
    virtual void Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus) override;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    bool bApproved = false;
    bool bCompleted = false;
    FGuid ItemId;
    FString CachedActorName;
    bool bItemIdCached = false;

    // Кэширует имя и ItemId из ActorRef при инициализации (если актор существует)
    void CacheActorInfo();
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorObjectDestroyedCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<AActor> ActorRef;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bShouldBeDestroyed = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    FGuid InitialItemId;

    virtual void Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus) override;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    bool bApproved = false;
    bool bCompleted = false;
    FGuid ItemId;
    FString CachedActorName;
    bool bItemIdCached = false;

    void CacheActorInfo();
};