#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "ChoreEnums.h"
#include "CheckRequestPayload.h"
#include "ChoreTimeSinceConditionAsset.generated.h"

UENUM(BlueprintType)
enum class EChoreTimeTrigger : uint8
{
    AfterAnyCompletion UMETA(DisplayName = "After chore completed"),
    AfterAnyFailure    UMETA(DisplayName = "After chore failed"),
    AfterAnyExpiry     UMETA(DisplayName = "After chore expired"),
    AfterAnyAbandon    UMETA(DisplayName = "After chore abandoned"),
    AfterAnyResult     UMETA(DisplayName = "After any chore result")
};

UCLASS(BlueprintType, ShowCategories = ("Time Since", "4 - Debug"))
class FPSKITALSREFACTORED_API UChoreTimeSinceConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    // Какой именно результат считается триггером.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since")
    EChoreTimeTrigger Trigger = EChoreTimeTrigger::AfterAnyCompletion;

    // Конкретная хора-источник. NAME_None — любая хора.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since")
    FName SourceChoreId;

    // Фильтр по семейству (используется, когда SourceChoreId не задан).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since")
    bool bUseFamily = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since",
        meta = (EditCondition = "bUseFamily", EditConditionHides))
    EChoreFamily Family = EChoreFamily::Miscellaneous;

    // Фильтр по подтипу.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since")
    bool bUseSubtype = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since",
        meta = (EditCondition = "bUseSubtype", EditConditionHides))
    EChoreSubtype Subtype = EChoreSubtype::Default;

    // Задержка после срабатывания триггера.
    // Дизайнер задаёт её в минутах и секундах — итог = Minutes * 60 + Seconds.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since", meta = (ClampMin = 0))
    int32 DelayMinutes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Since", meta = (ClampMin = 0.0f))
    float DelaySeconds = 0.0f;

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    // Разворачивает Trigger в список разрешённых EOutcomeChore.
    void BuildAllowedResults(TArray<EOutcomeChore>& OutResults) const;

    // Суммарная задержка в виде FTimespan.
    FTimespan GetRequiredDelay() const;
};