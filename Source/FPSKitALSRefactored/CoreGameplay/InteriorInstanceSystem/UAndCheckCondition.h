// UAndCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "UAndCheckCondition.generated.h"

/**
 * Условие, которое выполняет логическое И (AND) над массивом подчинённых условий.
 *
 * Работает асинхронно: запускает все подчинённые условия.
 * Если хотя бы одно подчинённое условие возвращает false, AND завершается досрочно с false.
 * Если все условия завершились с true, результат true.
 *
 * Поддерживает вложенность и синхронные условия.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UAndCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Массив подчинённых условий (объединённых по AND)
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
    // Обработчик завершения каждого подчинённого условия
    void OnSubConditionComplete(UCheckCondition* SubCondition);

    // Счётчик завершённых подчинённых условий
    int32 CompletedCount = 0;

    // Общее количество валидных подчинённых условий
    int32 TotalValidConditions = 0;

    // Флаг, что результат уже окончательный (чтобы не обрабатывать повторные завершения)
    bool bFinalized = false;
};