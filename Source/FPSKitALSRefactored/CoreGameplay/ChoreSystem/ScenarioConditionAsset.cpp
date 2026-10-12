#include "ScenarioConditionAsset.h"
#include "ScenarioManagerSubsystem/PayloadBatchAsset.h"
#include "ScenarioManagerSubsystem/ScenarioManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// Доступ к ScenarioManagerSubsystem через world contexts.
// ─────────────────────────────────────────────────────────────────────────────
static UScenarioManagerSubsystem* FindScenarioManager()
{
    if (!GEngine) return nullptr;

    for (const FWorldContext& C : GEngine->GetWorldContexts())
    {
        UWorld* W = C.World();
        if (!W || !W->IsGameWorld()) continue;

        if (UGameInstance* GI = W->GetGameInstance())
        {
            if (UScenarioManagerSubsystem* M = GI->GetSubsystem<UScenarioManagerSubsystem>())
                return M;
        }
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// CompileCondition — только описание для логов/редактора.
// ─────────────────────────────────────────────────────────────────────────────
void UScenarioConditionAsset::CompileCondition()
{
    class FScenarioCondition : public IOutcomeCondition
    {
    public:
        FScenarioCondition(const UScenarioConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("Scenario: Invalid");

            const FString ModeStr = StaticEnum<EScenarioConditionMode>()
                ->GetValueAsString(Asset->Mode);

            FString StepsStr;
            for (const TObjectPtr<UPayloadBatchAsset>& Step : Asset->Steps)
            {
                if (!Step) continue;
                if (!StepsStr.IsEmpty()) StepsStr += TEXT(", ");
                StepsStr += Step->GetName();
            }
            if (StepsStr.IsEmpty()) StepsStr = TEXT("<empty>");

            return FString::Printf(TEXT("Scenario: [%s] {%s}"), *ModeStr, *StepsStr);
        }
    private:
        const UScenarioConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FScenarioCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

// ─────────────────────────────────────────────────────────────────────────────
// EvaluateCondition — чисто state-driven. Outcome игнорируется.
// ─────────────────────────────────────────────────────────────────────────────
bool UScenarioConditionAsset::EvaluateCondition(const FOutcomeEventBase& /*Outcome*/) const
{
    if (Steps.Num() == 0) return false;

    UScenarioManagerSubsystem* ScenarioMgr = FindScenarioManager();
    if (!ScenarioMgr) return false;

    int32 ActiveCount = 0;
    int32 ValidCount  = 0;

    for (const TObjectPtr<UPayloadBatchAsset>& Step : Steps)
    {
        if (!Step) continue;
        ++ValidCount;

        if (ScenarioMgr->IsStepActive(Step))
            ++ActiveCount;
    }

    // Все элементы массива — null: нечего проверять.
    if (ValidCount == 0) return false;

    switch (Mode)
    {
    case EScenarioConditionMode::AnyActive:
        return ActiveCount > 0;

    case EScenarioConditionMode::AllActive:
        return ActiveCount == ValidCount;

    case EScenarioConditionMode::NoneActive:
        return ActiveCount == 0;

    default:
        return false;
    }
}