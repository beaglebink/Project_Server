#pragma once
#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ChoreEnums.h"
#include "CheckRequestPayload.h"
#include "WorldStateTypes.h"
#include "Outcome.h"
#include "WorldStateConditionAsset.generated.h"

UENUM(BlueprintType)
enum class EWorldStateConditionType : uint8
{
    // ---- State-driven (проверяется по текущему состоянию, событие не важно) ----
    FactExists          UMETA(DisplayName = "Fact Exists (state)"),
    CategoryExists      UMETA(DisplayName = "Any Fact of Category Exists (state)"),
    ValueMatches        UMETA(DisplayName = "Fact Value Matches (state)"),

    // ---- Event-driven (срабатывает на событии подсистемы) ----
    FactAdded           UMETA(DisplayName = "Fact Added (event)"),
    FactChanged         UMETA(DisplayName = "Fact Changed (event)"),
    FactRemoved         UMETA(DisplayName = "Fact Removed (event)")
};

UCLASS(BlueprintType, ShowCategories = ("WorldState", "4 - Debug"))
class FPSKITALSREFACTORED_API UWorldStateConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    EWorldStateConditionType ConditionType = EWorldStateConditionType::FactExists;

    // FactId — основной ключ для FactExists / FactAdded / FactChanged /
    // FactRemoved / ValueMatches. Для CategoryExists не используется.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType != EWorldStateConditionType::CategoryExists"))
    FName FactId;

    // Категория — для CategoryExists. Для event-driven типов может
    // использоваться как дополнительный фильтр (см. bMatchByCategory).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState")
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    // Дополнительный фильтр для event-driven типов: если true,
    // событие должно относиться к факту с указанной Category.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::FactAdded || ConditionType == EWorldStateConditionType::FactChanged || ConditionType == EWorldStateConditionType::FactRemoved"))
    bool bMatchByCategory = false;

    // Ожидаемое значение — используется только в ValueMatches.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::ValueMatches"))
    FString ExpectedValue;

    // Способ сравнения — используется только в ValueMatches.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::ValueMatches"))
    ECheckCompareOp CompareOp = ECheckCompareOp::Equal;

    virtual void CompileCondition() override;

    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    // Проверка события WorldStateFact* с учётом FactId / Category.
    bool MatchFactEvent(const FOutcomeEventBase& Outcome, EOutcomeWorldState ExpectedType) const;
};