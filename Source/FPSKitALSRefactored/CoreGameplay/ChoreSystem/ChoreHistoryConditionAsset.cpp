#include "ChoreHistoryConditionAsset.h"
#include "ChoreManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UChoreHistoryConditionAsset::CompileCondition()
{
    // Создаём условие-обёртку, которое вызывает EvaluateCondition
    class FChoreHistoryCondition : public IOutcomeCondition
    {
    public:
        FChoreHistoryCondition(const UChoreHistoryConditionAsset* InAsset) : Asset(InAsset) {}
        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }
        virtual FString Describe() const override
        {
            if (Asset)
            {
                return FString::Printf(TEXT("ChoreHistory: %s (%s) %s %d"),
                    *StaticEnum<EChoreHistoryQueryType>()->GetValueAsString(Asset->QueryType),
                    *Asset->GetName(),
                    *StaticEnum<ECheckCompareOp>()->GetValueAsString(Asset->CompareOp),
                    Asset->Threshold);
            }
            return TEXT("Invalid");
        }
    private:
        const UChoreHistoryConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FChoreHistoryCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

bool UChoreHistoryConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    // Получаем ChoreManager через GEngine и WorldContexts
    UChoreManagerSubsystem* ChoreManager = nullptr;
    if (GEngine)
    {
        const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
        for (const FWorldContext& Context : WorldContexts)
        {
            UWorld* World = Context.World();
            if (World && World->IsGameWorld())
            {
                UGameInstance* GI = World->GetGameInstance();
                if (GI)
                {
                    ChoreManager = GI->GetSubsystem<UChoreManagerSubsystem>();
                    if (ChoreManager) break;
                }
            }
        }
    }

    if (!ChoreManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("EvaluateCondition: ChoreManagerSubsystem is null"));
        return false;
    }

    int32 ActualValue = 0;

    switch (QueryType)
    {
    case EChoreHistoryQueryType::CountCompleted:
        ActualValue = ChoreManager->GetHistoryCount(ChoreId, Family, Subtype, bUseFamily, bUseSubtype, false);
        break;
    case EChoreHistoryQueryType::CountSucceeded:
        ActualValue = ChoreManager->GetHistoryCount(ChoreId, Family, Subtype, bUseFamily, bUseSubtype, true);
        break;
    case EChoreHistoryQueryType::BestPerformance:
    {
        float Best = ChoreManager->GetBestPerformance(ChoreId, Family, Subtype, bUseFamily, bUseSubtype, PerformanceMetricName.ToString());
        ActualValue = FMath::RoundToInt(Best);
        break;
    }
    case EChoreHistoryQueryType::LastResult:
    {
        bool bLastSucceeded = ChoreManager->GetLastResult(ChoreId);
        ActualValue = bLastSucceeded ? 1 : 0;
        break;
    }
    default:
        return false;
    }

    switch (CompareOp)
    {
    case ECheckCompareOp::Equal:          return ActualValue == Threshold;
    case ECheckCompareOp::NotEqual:       return ActualValue != Threshold;
    case ECheckCompareOp::Less:           return ActualValue < Threshold;
    case ECheckCompareOp::LessOrEqual:    return ActualValue <= Threshold;
    case ECheckCompareOp::Greater:        return ActualValue > Threshold;
    case ECheckCompareOp::GreaterOrEqual: return ActualValue >= Threshold;
    default: return false;
    }
}
