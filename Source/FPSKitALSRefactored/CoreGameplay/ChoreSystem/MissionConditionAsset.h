// MissionConditionAsset.h
#pragma once
#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ChoreEnums.h"
#include "CheckRequestPayload.h"
#include "MissionConditionAsset.generated.h"

UENUM(BlueprintType)
enum class EMissionConditionType : uint8
{
    IsCompleted    UMETA(DisplayName = "Mission Completed"),
    IsFailed       UMETA(DisplayName = "Mission Failed"),
    IsAbandoned    UMETA(DisplayName = "Mission Abandoned"),
    StepReached    UMETA(DisplayName = "Mission Step Reached")
};

UCLASS(BlueprintType, ShowCategories = ("Mission", "4 - Debug"))
class FPSKITALSREFACTORED_API UMissionConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    TObjectPtr<class UMissionAsset> MissionAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
    EMissionConditionType ConditionType = EMissionConditionType::IsCompleted;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission", meta = (EditCondition = "ConditionType == EMissionConditionType::StepReached"))
    int32 StepIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission", meta = (EditCondition = "ConditionType == EMissionConditionType::StepReached"))
    ECheckCompareOp CompareOp = ECheckCompareOp::Equal;

    virtual void CompileCondition() override;

    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    FName GetEffectiveMissionId() const;
    FText GetDisplayName() const;
};