#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ScenarioConditionAsset.generated.h"

class UPayloadBatchAsset;

UENUM(BlueprintType)
enum class EScenarioConditionMode : uint8
{
    // Истина, если активен хотя бы один из указанных шагов.
    // Типичный случай: «доступно, пока сценарий X активен».
    AnyActive  UMETA(DisplayName = "Any of specified is active"),

    // Истина, если активны ВСЕ указанные шаги.
    // Типичный случай: «доступно только при комбинации шагов».
    AllActive  UMETA(DisplayName = "All specified are active"),

    // Истина, если ни один из указанных шагов не активен.
    // Типичный случай: «доступно после того, как сценарий X закончился».
    NoneActive UMETA(DisplayName = "None of specified is active")
};

/**
 * Условие по сценарию (ScenarioManagerSubsystem).
 *
 * Проверяет, какие из указанных UPayloadBatchAsset-шагов сейчас входят
 * в ActiveStepIds менеджера сценариев.
 *
 * Условие state-driven: не зависит от приходящего события, пересчитывается
 * при каждом Evaluate. В ChoreManagerSubsystem дополнительно переоценивается
 * по pulse-таймеру, что позволяет реагировать на изменения состояния
 * сценария без явного события.
 *
 * Примеры использования в AvailabilityCondition хоры:
 *   • Mode = AnyActive,  Steps = [StepA, StepB]  → хора доступна, пока активен StepA или StepB.
 *   • Mode = NoneActive, Steps = [StepA]         → хора доступна, когда StepA не активен.
 *   • Mode = AllActive,  Steps = [StepA, StepB]  → хора доступна, только если активны оба шага.
 */
UCLASS(BlueprintType, ShowCategories = ("Scenario", "4 - Debug"))
class FPSKITALSREFACTORED_API UScenarioConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    // Как объединять несколько указанных шагов.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario")
    EScenarioConditionMode Mode = EScenarioConditionMode::AnyActive;

    // Набор шагов сценария (UPayloadBatchAsset).
    // Пустой массив даёт false во всех режимах — нечего проверять.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scenario")
    TArray<TObjectPtr<UPayloadBatchAsset>> Steps;

    // Условие чисто state-driven: не зависит от события, читает текущее
    // состояние ScenarioManagerSubsystem. Это даёт ChoreManager-у право
    // переоценивать его по pulse без приходящего события.
    virtual bool IsStateDriven() const override { return true; }

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;
};