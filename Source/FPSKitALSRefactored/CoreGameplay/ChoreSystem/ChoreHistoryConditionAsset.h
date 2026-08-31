#pragma once
#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ChoreEnums.h"
#include "CheckRequestPayload.h"
#include "ChoreHistoryConditionAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreHistoryConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    EChoreHistoryQueryType QueryType = EChoreHistoryQueryType::CountSucceeded;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    FName ChoreId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    EChoreFamily Family = EChoreFamily::Miscellaneous;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    EChoreSubtype Subtype = EChoreSubtype::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    bool bUseFamily = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    bool bUseSubtype = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    int32 Threshold = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History", meta = (EditCondition = "QueryType == EChoreHistoryQueryType::BestPerformance"))
    FName PerformanceMetricName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "History")
    ECheckCompareOp CompareOp = ECheckCompareOp::GreaterOrEqual;

    // Переопределяем метод компиляции (без override, т.к. в базовом классе он не виртуальный)
    virtual void CompileCondition();

    // Метод проверки условия (вызывается из скомпилированного условия)
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;
};