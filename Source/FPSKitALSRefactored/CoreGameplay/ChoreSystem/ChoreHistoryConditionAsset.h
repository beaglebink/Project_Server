#pragma once
#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ChoreEnums.h"
#include "CheckRequestPayload.h"
#include "ChoreHistoryConditionAsset.generated.h"

UCLASS(BlueprintType, ShowCategories = ("History", "4 - Debug"))
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

    // Метод для получения строкового описания фильтра
    UFUNCTION(BlueprintCallable, Category = "ChoreHistory")
    FString GetFilterDescription() const;

    // Переопределяем метод компиляции
    virtual void CompileCondition() override;

    // Метод проверки условия (вызывается из скомпилированного условия)
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;
};