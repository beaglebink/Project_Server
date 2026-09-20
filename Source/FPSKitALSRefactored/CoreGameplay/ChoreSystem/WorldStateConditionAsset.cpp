#include "WorldStateConditionAsset.h"
#include "WorldStateSubsystem.h"
#include "WorldStateFactChangedPayload.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/DefaultValueHelper.h"

// ─────────────────────────────────────────────────────────────────────────────
// Вспомогательный доступ к подсистеме через контексты мира.
// ─────────────────────────────────────────────────────────────────────────────
static UWorldStateSubsystem* GetWorldStateSubsystem()
{
    if (!GEngine) return nullptr;

    const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
    for (const FWorldContext& Context : WorldContexts)
    {
        UWorld* World = Context.World();
        if (World && World->IsGameWorld())
        {
            UGameInstance* GI = World->GetGameInstance();
            if (GI)
            {
                if (UWorldStateSubsystem* Sub = GI->GetSubsystem<UWorldStateSubsystem>())
                {
                    return Sub;
                }
            }
        }
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// CompileCondition: описываем условие для отладки/логов.
// ─────────────────────────────────────────────────────────────────────────────
void UWorldStateConditionAsset::CompileCondition()
{
    class FWorldStateCondition : public IOutcomeCondition
    {
    public:
        FWorldStateCondition(const UWorldStateConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("WorldState: Invalid");

            const FString CondStr = StaticEnum<EWorldStateConditionType>()
                ->GetValueAsString(Asset->ConditionType);

            switch (Asset->ConditionType)
            {
            case EWorldStateConditionType::FactExists:
                return FString::Printf(TEXT("WorldState: [%s] FactId='%s'"),
                    *CondStr, *Asset->FactId.ToString());

            case EWorldStateConditionType::CategoryExists:
                return FString::Printf(TEXT("WorldState: [%s] Category=%s"),
                    *CondStr,
                    *StaticEnum<EWorldStateChangeCategory>()->GetValueAsString(Asset->Category));

            case EWorldStateConditionType::ValueMatches:
                return FString::Printf(TEXT("WorldState: [%s] FactId='%s' Value %s '%s'"),
                    *CondStr, *Asset->FactId.ToString(),
                    *StaticEnum<ECheckCompareOp>()->GetValueAsString(Asset->CompareOp),
                    *Asset->ExpectedValue);

            case EWorldStateConditionType::FactAdded:
            case EWorldStateConditionType::FactChanged:
            case EWorldStateConditionType::FactRemoved:
                return FString::Printf(TEXT("WorldState: [%s] FactId='%s' Category=%s (byCategory=%s)"),
                    *CondStr, *Asset->FactId.ToString(),
                    *StaticEnum<EWorldStateChangeCategory>()->GetValueAsString(Asset->Category),
                    Asset->bMatchByCategory ? TEXT("true") : TEXT("false"));

            default:
                return FString::Printf(TEXT("WorldState: [%s]"), *CondStr);
            }
        }

    private:
        const UWorldStateConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FWorldStateCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

// ─────────────────────────────────────────────────────────────────────────────
// Оценка условия
// ─────────────────────────────────────────────────────────────────────────────
bool UWorldStateConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    UWorldStateSubsystem* WorldState = GetWorldStateSubsystem();
    if (!WorldState)
        return false;

    switch (ConditionType)
    {
    // ---- State-driven ----

    case EWorldStateConditionType::FactExists:
    {
        if (FactId.IsNone()) return false;
        // bIncludePendingRemoval = false → записи, помеченные на удаление,
        // считаются уже отсутствующими.
        return WorldState->HasWorldStateRecord(FactId, /*bIncludePendingRemoval=*/false);
    }

    case EWorldStateConditionType::CategoryExists:
    {
        const TArray<FWorldStateRecord> Records =
            WorldState->GetRecordsByCategory(Category, /*bIncludePendingRemoval=*/false);
        return Records.Num() > 0;
    }

    case EWorldStateConditionType::ValueMatches:
    {
        if (FactId.IsNone()) return false;

        const FWorldStateRecord* Record =
            WorldState->FindWorldStateRecord(FactId, /*bIncludePendingRemoval=*/false);
        if (!Record) return false;

        // Значения хранятся как строки (результат ExportText). Для числовых
        // сравнений приводим обе стороны к double, если это возможно.
        const FString& Actual = Record->SerializedValue;

        auto TryParseNum = [](const FString& S, double& Out) -> bool
        {
            return FDefaultValueHelper::ParseDouble(S, Out);
        };

        double ExpectedNum = 0.0, ActualNum = 0.0;
        const bool bBothNumeric =
            TryParseNum(ExpectedValue, ExpectedNum) &&
            TryParseNum(Actual, ActualNum);

        switch (CompareOp)
        {
        case ECheckCompareOp::Equal:
            return bBothNumeric
                ? FMath::IsNearlyEqual(ActualNum, ExpectedNum)
                : Actual.Equals(ExpectedValue, ESearchCase::CaseSensitive);

        case ECheckCompareOp::NotEqual:
            return bBothNumeric
                ? !FMath::IsNearlyEqual(ActualNum, ExpectedNum)
                : !Actual.Equals(ExpectedValue, ESearchCase::CaseSensitive);

        case ECheckCompareOp::Less:
            return bBothNumeric && ActualNum < ExpectedNum;

        case ECheckCompareOp::LessOrEqual:
            return bBothNumeric && ActualNum <= ExpectedNum;

        case ECheckCompareOp::Greater:
            return bBothNumeric && ActualNum > ExpectedNum;

        case ECheckCompareOp::GreaterOrEqual:
            return bBothNumeric && ActualNum >= ExpectedNum;

        default:
            return false;
        }
    }

    // ---- Event-driven ----

    case EWorldStateConditionType::FactAdded:
        return MatchFactEvent(Outcome, EOutcomeWorldState::WorldStateFactAdded);

    case EWorldStateConditionType::FactChanged:
        return MatchFactEvent(Outcome, EOutcomeWorldState::WorldStateFactChanged);

    case EWorldStateConditionType::FactRemoved:
        return MatchFactEvent(Outcome, EOutcomeWorldState::WorldStateFactRemoved);

    default:
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Event-driven matching
// ─────────────────────────────────────────────────────────────────────────────
bool UWorldStateConditionAsset::MatchFactEvent(const FOutcomeEventBase& Outcome, EOutcomeWorldState ExpectedType) const
{
    if (Outcome.OutcomeType != EOutcomeType::WorldState)
        return false;
    if (Outcome.OutcomeWorldState != ExpectedType)
        return false;

    const UWorldStateFactChangedPayload* P =
        Cast<UWorldStateFactChangedPayload>(Outcome.Payload);
    if (!P)
        return false;

    if (!FactId.IsNone() && P->FactId != FactId)
        return false;

    if (bMatchByCategory && P->Record.Category != Category)
        return false;

    return true;
}