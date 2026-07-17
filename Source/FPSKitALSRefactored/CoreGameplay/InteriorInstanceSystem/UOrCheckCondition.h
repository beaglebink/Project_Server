// UOrCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "UOrCheckCondition.generated.h"

/**
 * Условие, которое выполняет логическое ИЛИ (OR) над массивом подчинённых условий.
 *
 * Завершается досрочно с true, как только любое подусловие вернёт true.
 * Если все подусловия завершились с false, возвращает false.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UOrCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced)
    TArray<UCheckCondition*> SubConditions;

    virtual void Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus) override;
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual void Reset() override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    void OnSubConditionComplete(UCheckCondition* SubCondition);

    int32 CompletedCount = 0;
    int32 TotalValidConditions = 0;
    bool bFinalized = false; // защита от повторных завершений
};