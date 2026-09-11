#include "ChoreHistoryConditionAsset.h"
#include "ChoreManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// ---- Утилита миграции (используется в PostLoad/PostEditChangeProperty) ----
static EChorePerformanceMetric MetricFromLegacyName(const FName& LegacyName)
{
    if (LegacyName == TEXT("Mistakes"))              return EChorePerformanceMetric::Mistakes;
    if (LegacyName == TEXT("Accuracy"))              return EChorePerformanceMetric::Accuracy;
    if (LegacyName == TEXT("Quantity"))              return EChorePerformanceMetric::Quantity;
    if (LegacyName == TEXT("CompletionTimeSeconds")) return EChorePerformanceMetric::CompletionTimeSeconds;
    // По умолчанию — самая «естественная» метрика для истории хор.
    return EChorePerformanceMetric::CompletionTimeSeconds;
}

void UChoreHistoryConditionAsset::CompileCondition()
{
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
                FString FilterDesc = Asset->GetFilterDescription();
                FString MetricDesc;
                if (Asset->QueryType == EChoreHistoryQueryType::BestPerformance)
                {
                    MetricDesc = FString::Printf(TEXT("[%s] "),
                        *StaticEnum<EChorePerformanceMetric>()->GetValueAsString(Asset->Metric));
                }
                return FString::Printf(TEXT("ChoreHistory: %s (%s) %s%s %d"),
                    *StaticEnum<EChoreHistoryQueryType>()->GetValueAsString(Asset->QueryType),
                    *FilterDesc,
                    *MetricDesc,
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

FString UChoreHistoryConditionAsset::GetFilterDescription() const
{
    if (bUseFamily && bUseSubtype)
    {
        return FString::Printf(TEXT("%s / %s"),
            *StaticEnum<EChoreFamily>()->GetValueAsString(Family),
            *StaticEnum<EChoreSubtype>()->GetValueAsString(Subtype));
    }
    else if (bUseFamily)
    {
        return StaticEnum<EChoreFamily>()->GetValueAsString(Family);
    }
    else if (bUseSubtype)
    {
        return StaticEnum<EChoreSubtype>()->GetValueAsString(Subtype);
    }
    else if (!ChoreId.IsNone())
    {
        return ChoreId.ToString();
    }
    else
    {
        return TEXT("No filter");
    }
}

bool UChoreHistoryConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    // Проверка наличия фильтра
    if (ChoreId.IsNone() && !bUseFamily && !bUseSubtype)
    {
        UE_LOG(LogTemp, Warning, TEXT("EvaluateCondition: ChoreHistoryConditionAsset '%s' has no filter, returning false"), *GetName());
        return false;
    }

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
        // ---- НОВОЕ: передаём enum вместо строки ----
        float Best = ChoreManager->GetBestPerformance(ChoreId, Family, Subtype, bUseFamily, bUseSubtype, Metric);
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

// ---- Миграция старого FName-поля в новый enum ----
#if WITH_EDITOR
void UChoreHistoryConditionAsset::PostLoad()
{
    Super::PostLoad();

    // Если старое поле задано — переносим значение в Metric и очищаем legacy.
    if (!PerformanceMetricName_DEPRECATED.IsNone())
    {
        Metric = MetricFromLegacyName(PerformanceMetricName_DEPRECATED);
        PerformanceMetricName_DEPRECATED = NAME_None;
        // Помечаем пакет как изменённый, чтобы при следующем сохранении ассета
        // значение ушло в Metric.
        MarkPackageDirty();
    }
}

void UChoreHistoryConditionAsset::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);

    // На случай, если legacy-поле откуда-то появилось в редакторе — тоже мигрируем.
    if (!PerformanceMetricName_DEPRECATED.IsNone())
    {
        Metric = MetricFromLegacyName(PerformanceMetricName_DEPRECATED);
        PerformanceMetricName_DEPRECATED = NAME_None;
    }
}
#endif