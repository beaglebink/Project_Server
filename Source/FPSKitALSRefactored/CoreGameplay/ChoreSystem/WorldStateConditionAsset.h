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

    // FactId — для FactExists / ValueMatches (state-driven) и для
    // FactAdded / FactChanged / FactRemoved при bMatchByCategory = false.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::FactExists || ConditionType == EWorldStateConditionType::ValueMatches || ((ConditionType == EWorldStateConditionType::FactAdded || ConditionType == EWorldStateConditionType::FactChanged || ConditionType == EWorldStateConditionType::FactRemoved) && !bMatchByCategory)",
            EditConditionHides))
    FName FactId;

    // Category — для CategoryExists (state-driven) и для
    // FactAdded / FactChanged / FactRemoved при bMatchByCategory = true.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::CategoryExists || ((ConditionType == EWorldStateConditionType::FactAdded || ConditionType == EWorldStateConditionType::FactChanged || ConditionType == EWorldStateConditionType::FactRemoved) && bMatchByCategory)",
            EditConditionHides))
    EWorldStateChangeCategory Category = EWorldStateChangeCategory::Custom;

    // Дополнительный фильтр для event-driven типов: если true,
    // фильтрация идёт по Category, а FactId игнорируется.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::FactAdded || ConditionType == EWorldStateConditionType::FactChanged || ConditionType == EWorldStateConditionType::FactRemoved",
            EditConditionHides))
    bool bMatchByCategory = false;

    // Ожидаемое значение — используется только в ValueMatches.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::ValueMatches",
            EditConditionHides))
    FString ExpectedValue;

    // Способ сравнения — используется только в ValueMatches.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldState",
        meta = (EditCondition = "ConditionType == EWorldStateConditionType::ValueMatches",
            EditConditionHides))
    ECheckCompareOp CompareOp = ECheckCompareOp::Equal;

    virtual void CompileCondition() override;

    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    bool MatchFactEvent(const FOutcomeEventBase& Outcome, EOutcomeWorldState ExpectedType) const;
};