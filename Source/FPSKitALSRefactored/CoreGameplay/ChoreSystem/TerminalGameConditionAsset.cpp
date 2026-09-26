#include "TerminalGameConditionAsset.h"
#include "../TerminalSystem/TerminalSubsystem.h"   // ETerminalRecordStatus, ETerminalStageResult, FTerminalActivityRecord
#include "CheckRequestPayload.h"                   // ECheckCompareOp
#include "Outcome.h"                               // EOutcomeType
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
            // Условие чисто state-driven и не зависит от конкретного Outcome,
            // но вызов приходит с любым событием — просто игнорируем его параметры.
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

            case ETerminalGameQueryType::StageFinished:
                return FString::Printf(TEXT("TerminalGame: [%s] Stage='%s' Result=%s %s / %s"),
                    *QueryStr, *Asset->StageId,
                    *StaticEnum<ETerminalStageResult>()->GetValueAsString(Asset->StageResult),
                    *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::AllStagesCompleted:
                return FString::Printf(TEXT("TerminalGame: [%s] %s / %s"),
                    *QueryStr, *TargetDesc, *GameDesc);

            case ETerminalGameQueryType::ScoreReached:
                return FString::Printf(TEXT("TerminalGame: [%s] Score %s %d %s / %s"),
                    *QueryStr,
                    *StaticEnum<ECheckCompareOp>()->GetValueAsString(Asset->ScoreCompareOp),
                    Asset->ScoreThreshold,
                    *TargetDesc, *GameDesc);

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
            return false;
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
        // Любая запись, кроме "NotStarted", считается существующей.
        return Record.Status != ETerminalRecordStatus::NotStarted;

    case ETerminalGameQueryType::ReachedStatus:
        return Record.Status == Status;

    case ETerminalGameQueryType::StageFinished:
    {
        if (StageId.IsEmpty()) return false;

        for (const FTerminalStageProgress& S : Record.Stages)
        {
            if (S.StageId == StageId && S.Result == StageResult)
                return true;
        }
        return false;
    }

    case ETerminalGameQueryType::AllStagesCompleted:
    {
        if (Record.Stages.Num() == 0) return false;

        for (const FTerminalStageProgress& S : Record.Stages)
        {
            if (S.Result != ETerminalStageResult::Completed) return false;
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
    if (bUseTerminalFilter && !ActivityId.IsEmpty())
    {
        FGuid ParsedId;
        if (!FGuid::Parse(TerminalIdString, ParsedId))
            return Result;

        FTerminalActivityRecord Rec;
        if (Terminal->GetGameRecord(ParsedId, ActivityId, Rec))
            Result.Add(Rec);
        return Result;
    }

    if (bUseTerminalFilter)
    {
        FGuid ParsedId;
        if (!FGuid::Parse(TerminalIdString, ParsedId))
            return Result;
        return Terminal->GetGameRecordsForTerminal(ParsedId);
    }

    if (!ActivityId.IsEmpty())
        return Terminal->GetAllGameRecordsByActivityId(ActivityId);

    return Terminal->GetAllGameRecordsAcrossTerminals();
}

// ─────────────────────────────────────────────────────────────────────────────
// Главный evaluator
//
// Условие чисто state-driven: Outcome не используется. Параметр оставлен для
// совместимости с IOutcomeCondition::Evaluate, но игнорируется.
// ─────────────────────────────────────────────────────────────────────────────
bool UTerminalGameConditionAsset::EvaluateCondition(const FOutcomeEventBase& /*Outcome*/) const
{
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