// UNotCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "UNotCheckCondition.generated.h"

/**
 * Условие, которое инвертирует результат одного подчинённого условия (логическое НЕ).
 *
 * Работает асинхронно: запускает подчинённое условие и ждёт его завершения,
 * после чего возвращает инвертированный результат (true → false, false → true).
 *
 * Полезно для комбинаций: NOT( A ) ИЛИ NOT( B ) и т.п.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UNotCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Единственное подчинённое условие, результат которого инвертируется
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
    // Обработчик завершения подчинённого условия
    void OnInnerConditionComplete(UCheckCondition* Condition);

    bool bApproved = false;
    bool bCompleted = false;
};