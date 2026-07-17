// UNotCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "UNotCheckCondition.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UNotCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
    UCheckCondition* InnerCondition = nullptr;

    virtual void Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus) override;
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual void Reset() override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    void OnInnerConditionComplete(UCheckCondition* Condition);

    bool bApproved = false;
    bool bCompleted = false;
    bool bFinalized = false;
};