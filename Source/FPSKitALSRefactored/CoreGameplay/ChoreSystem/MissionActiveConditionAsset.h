#pragma once
#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ChoreEnums.h"
#include "MissionActiveConditionAsset.generated.h"

UCLASS(BlueprintType, HideCategories = ("1 - Operator", "2 - Composite Conditions", "2 - Logic Operands", "3 - Simple Condition"))
class FPSKITALSREFACTORED_API UMissionActiveConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    // Миссия, активность которой нужно проверить
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    TObjectPtr<class UMissionAsset> MissionAsset;

    // Ожидаемое состояние: true – миссия должна быть активна, false – не активна
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    bool ExpectedActive = true;

    virtual void CompileCondition() override;

    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    FName GetEffectiveMissionId() const;
    FText GetDisplayName() const;
};