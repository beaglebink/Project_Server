#include "TerminalGameConditionAsset.h"
#include "../TerminalSystem/TerminalSubsystem.h"   // ETerminalRecordStatus, FTerminalActivityRecord
#include "../TerminalSystem/TerminalTaskPayload.h" // UTerminalActivityRecordChangedPayload, UTerminalRemoveActivityRecordPayload
#include "CheckRequestPayload.h"                   // ECheckCompareOp
#include "Outcome.h"                               // EOutcomeType, EOutcomeTerminal
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// Доступ к подсистеме через контексты мира.
// ─────────────────────────────────────────────────────────────────────────────
static UTerminalSubsystem* GetTerminalSubsystem()
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
                if (UTerminalSubsystem* Sub = GI->GetSubsystem<UTerminalSubsystem>())
                {
                    return Sub;
                }
            }
        }
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// CompileCondition — описание для логов / редактора.
// ─────────────────────────────────────────────────────────────────────────────
void UTerminalGameConditionAsset::CompileCondition()
{
    class FTerminalGameCondition : public IOutcomeCondition
    {
    public:
        FTerminalGameCondition(const UTerminalGameConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("TerminalGame: Invalid");

            const FString QueryStr = StaticEnum<ETerminalGameQueryType>()
                ->GetValueAsString(Asset->QueryType);

            FString TargetDesc = Asset->bUseTerminalFilter
                ? FString::Printf(TEXT("Terminal=%s"), *Asset->TerminalIdString)
                : TEXT("any terminal");

            FString GameDesc = Asset->ActivityId.IsEmpty()
                ? TEXT("any game")
                : FString::Printf(TEXT("Game='%s'"), *Asset->ActivityId);

            switch (Asset->QueryType)
            {
            case ETerminalGameQueryType::HasGameRecord:
                return FString::Printf(TEXT("TerminalGame: [%s] %s / %s"),
                    *QueryStr, *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::ReachedStatus:
                return FString::Printf(TEXT("TerminalGame: [%s] Status=%s %s / %s"),
                    *QueryStr,
                    *StaticEnum<ETerminalRecordStatus>()->GetValueAsString(Asset->Status),
                    *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::StageCompleted:
                return FString::Printf(TEXT("TerminalGame: [%s] Stage='%s' %s / %s"),
                    *QueryStr, *Asset->StageId, *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::AllStagesCompleted:
                return FString::Printf(TEXT("TerminalGame: [%s] %s / %s"),
                    *QueryStr, *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::ScoreReached:
                return FString::Printf(TEXT("TerminalGame: [%s] Score %s %d %s / %s"),
                    *QueryStr,
                    *StaticEnum<ECheckCompareOp>()->GetValueAsString(Asset->ScoreCompareOp),
                    Asset->ScoreThreshold,
                    *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::OnGameStatusChanged:
                return FString::Printf(TEXT("TerminalGame: [%s] -> %s %s / %s"),
                    *QueryStr,
                    *StaticEnum<ETerminalRecordStatus>()->GetValueAsString(Asset->Status),
                    *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::OnStageCompleted:
                return FString::Printf(TEXT("TerminalGame: [%s] Stage='%s' %s / %s"),
                    *QueryStr, *Asset->StageId, *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::OnGameRecordRemoved:
                return FString::Printf(TEXT("TerminalGame: [%s] %s / %s"),
                    *QueryStr, *TargetDesc, *GameDesc);

            default:
                return FString::Printf(TEXT("TerminalGame: [%s]"), *QueryStr);
            }
        }

    private:
        const UTerminalGameConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FTerminalGameCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

// ─────────────────────────────────────────────────────────────────────────────
// Фильтры и state-evaluation
// ─────────────────────────────────────────────────────────────────────────────
bool UTerminalGameConditionAsset::PassesCommonFilters(const FTerminalActivityRecord& Record) const
{
    if (bUseTerminalFilter)
    {
        FGuid ParsedId;
        if (!FGuid::Parse(TerminalIdString, ParsedId))
        {
            // Строка невалидна — фильтр не проходит.
            return false;
        }
        if (Record.TerminalId != ParsedId)
            return false;
    }

    if (!ActivityId.IsEmpty() && Record.ActivityId != ActivityId)
        return false;

    return true;
}

bool UTerminalGameConditionAsset::EvaluateRecordState(const FTerminalActivityRecord& Record) const
{
    switch (QueryType)
    {
    case ETerminalGameQueryType::HasGameRecord:
        // Любая запись с любым статусом, кроме "NotStarted" — формально
        // запись существует только если по игре что-то происходило.
        return Record.Status != ETerminalRecordStatus::NotStarted;

    case ETerminalGameQueryType::ReachedStatus:
        // Точное совпадение статуса.
        return Record.Status == Status;

    case ETerminalGameQueryType::StageCompleted:
    {
        if (StageId.IsEmpty()) return false;

        for (const FTerminalStageProgress& S : Record.Stages)
        {
            if (S.StageId == StageId && S.bCompleted)
                return true;
        }
        return false;
    }

    case ETerminalGameQueryType::AllStagesCompleted:
    {
        // Нет стадий — нечего завершать, условие не выполнено.
        if (Record.Stages.Num() == 0) return false;

        for (const FTerminalStageProgress& S : Record.Stages)
        {
            if (!S.bCompleted) return false;
        }
        return true;
    }

    case ETerminalGameQueryType::ScoreReached:
    {
        const int32 Actual = Record.TotalScore;
        switch (ScoreCompareOp)
        {
        case ECheckCompareOp::Equal:          return Actual == ScoreThreshold;
        case ECheckCompareOp::NotEqual:       return Actual != ScoreThreshold;
        case ECheckCompareOp::Less:           return Actual < ScoreThreshold;
        case ECheckCompareOp::LessOrEqual:    return Actual <= ScoreThreshold;
        case ECheckCompareOp::Greater:        return Actual > ScoreThreshold;
        case ECheckCompareOp::GreaterOrEqual: return Actual >= ScoreThreshold;
        default:                              return false;
        }
    }

    default:
        return false;
    }
}

TArray<FTerminalActivityRecord> UTerminalGameConditionAsset::CollectCandidateRecords() const
{
    TArray<FTerminalActivityRecord> Result;

    UTerminalSubsystem* Terminal = GetTerminalSubsystem();
    if (!Terminal) return Result;

    // Собираем максимально широко, затем фильтруем PassesCommonFilters.
    // Это дешевле, чем повторять сборку под 4 комбинации фильтров.
    if (bUseTerminalFilter && !ActivityId.IsEmpty())
    {
        FGuid ParsedId;
        if (!FGuid::Parse(TerminalIdString, ParsedId))
        {
            return Result; // невалидный GUID → пусто
        }

        FTerminalActivityRecord Rec;
        if (Terminal->GetGameRecord(ParsedId, ActivityId, Rec))
        {
            Result.Add(Rec);
        }
        return Result;
    }

    if (bUseTerminalFilter)
    {
        FGuid ParsedId;
        if (!FGuid::Parse(TerminalIdString, ParsedId))
        {
            return Result;
        }
        return Terminal->GetGameRecordsForTerminal(ParsedId);
    }

    if (!ActivityId.IsEmpty())
    {
        return Terminal->GetAllGameRecordsByActivityId(ActivityId);
    }

    return Terminal->GetAllGameRecordsAcrossTerminals();
}

// ─────────────────────────────────────────────────────────────────────────────
// Event matching
// ─────────────────────────────────────────────────────────────────────────────
bool UTerminalGameConditionAsset::MatchGameEvent(
    const FOutcomeEventBase& Outcome,
    EOutcomeTerminal ExpectedEvent,
    const TFunctionRef<bool(const FTerminalActivityRecord&)>& Predicate) const
{
    if (Outcome.OutcomeType != EOutcomeType::Terminal)
        return false;
    if (Outcome.OutcomeTerminal != ExpectedEvent)
        return false;

    if (ExpectedEvent == EOutcomeTerminal::GameRecordUpdated)
    {
        const UTerminalActivityRecordChangedPayload* P =
            Cast<UTerminalActivityRecordChangedPayload>(Outcome.Payload);
        if (!P) return false;

        if (!PassesCommonFilters(P->Record)) return false;
        return Predicate(P->Record);
    }

    if (ExpectedEvent == EOutcomeTerminal::GameRecordRemoved)
    {
        const UTerminalRemoveActivityRecordPayload* P =
            Cast<UTerminalRemoveActivityRecordPayload>(Outcome.Payload);
        if (!P) return false;

        // Синтетическая запись для прогона общих фильтров.
        FTerminalActivityRecord Proxy;
        Proxy.TerminalId = P->TerminalId;
        Proxy.ActivityId = P->ActivityId;

        if (!PassesCommonFilters(Proxy)) return false;
        return Predicate(Proxy);
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Главный evaluator
// ─────────────────────────────────────────────────────────────────────────────
bool UTerminalGameConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    // ---- Event-driven ----
    switch (QueryType)
    {
    case ETerminalGameQueryType::OnGameStatusChanged:
    {
        return MatchGameEvent(Outcome, EOutcomeTerminal::GameRecordUpdated,
            [this](const FTerminalActivityRecord& Rec)
            {
                return Rec.Status == Status;
            });
    }

    case ETerminalGameQueryType::OnStageCompleted:
    {
        if (StageId.IsEmpty()) return false;

        return MatchGameEvent(Outcome, EOutcomeTerminal::GameRecordUpdated,
            [this](const FTerminalActivityRecord& Rec)
            {
                for (const FTerminalStageProgress& S : Rec.Stages)
                {
                    if (S.StageId == StageId && S.bCompleted)
                        return true;
                }
                return false;
            });
    }

    case ETerminalGameQueryType::OnGameRecordRemoved:
    {
        return MatchGameEvent(Outcome, EOutcomeTerminal::GameRecordRemoved,
            [](const FTerminalActivityRecord&) { return true; });
    }

    default:
        break;
    }

    // ---- State-driven ----
    UTerminalSubsystem* Terminal = GetTerminalSubsystem();
    if (!Terminal) return false;

    const TArray<FTerminalActivityRecord> Candidates = CollectCandidateRecords();
    for (const FTerminalActivityRecord& Rec : Candidates)
    {
        if (!PassesCommonFilters(Rec)) continue;
        if (EvaluateRecordState(Rec)) return true;
    }
    return false;
}