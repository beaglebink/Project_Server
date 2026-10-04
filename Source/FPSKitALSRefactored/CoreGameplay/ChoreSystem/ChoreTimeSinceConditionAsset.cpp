#include "ChoreTimeSinceConditionAsset.h"
#include "ChoreManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// Доступ к ChoreManagerSubsystem через контексты мира.
// ─────────────────────────────────────────────────────────────────────────────
static UChoreManagerSubsystem* GetChoreManagerSubsystem()
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
                if (UChoreManagerSubsystem* Sub = GI->GetSubsystem<UChoreManagerSubsystem>())
                {
                    return Sub;
                }
            }
        }
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Утилиты
// ─────────────────────────────────────────────────────────────────────────────
void UChoreTimeSinceConditionAsset::BuildAllowedResults(TArray<EOutcomeChore>& OutResults) const
{
    OutResults.Reset();
    switch (Trigger)
    {
    case EChoreTimeTrigger::AfterAnyCompletion:
        OutResults.Add(EOutcomeChore::CompleteRequest);
        break;

    case EChoreTimeTrigger::AfterAnyFailure:
        OutResults.Add(EOutcomeChore::FailRequest);
        break;

    case EChoreTimeTrigger::AfterAnyExpiry:
        OutResults.Add(EOutcomeChore::ExpireRequest);
        break;

    case EChoreTimeTrigger::AfterAnyAbandon:
        OutResults.Add(EOutcomeChore::AbandonRequest);
        break;

    case EChoreTimeTrigger::AfterAnyResult:
        OutResults.Add(EOutcomeChore::CompleteRequest);
        OutResults.Add(EOutcomeChore::FailRequest);
        OutResults.Add(EOutcomeChore::ExpireRequest);
        OutResults.Add(EOutcomeChore::AbandonRequest);
        break;
    }
}

FTimespan UChoreTimeSinceConditionAsset::GetRequiredDelay() const
{
    FTimespan Result = FTimespan::FromMinutes(FMath::Max(0, DelayMinutes));
    Result += FTimespan::FromSeconds(FMath::Max(0.0f, DelaySeconds));
    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// CompileCondition — описание для логов и редактора.
// ─────────────────────────────────────────────────────────────────────────────
void UChoreTimeSinceConditionAsset::CompileCondition()
{
    class FChoreTimeSinceCondition : public IOutcomeCondition
    {
    public:
        FChoreTimeSinceCondition(const UChoreTimeSinceConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("TimeSince: Invalid");

            const FString TriggerStr = StaticEnum<EChoreTimeTrigger>()
                ->GetValueAsString(Asset->Trigger);

            FString SourceDesc;
            if (!Asset->SourceChoreId.IsNone())
            {
                SourceDesc = Asset->SourceChoreId.ToString();
            }
            else if (Asset->bUseFamily && Asset->bUseSubtype)
            {
                SourceDesc = FString::Printf(TEXT("%s / %s"),
                    *StaticEnum<EChoreFamily>()->GetValueAsString(Asset->Family),
                    *StaticEnum<EChoreSubtype>()->GetValueAsString(Asset->Subtype));
            }
            else if (Asset->bUseFamily)
            {
                SourceDesc = StaticEnum<EChoreFamily>()->GetValueAsString(Asset->Family);
            }
            else if (Asset->bUseSubtype)
            {
                SourceDesc = StaticEnum<EChoreSubtype>()->GetValueAsString(Asset->Subtype);
            }
            else
            {
                SourceDesc = TEXT("any chore");
            }

            const FTimespan Required = Asset->GetRequiredDelay();
            const int32 TotalMinutes = (int32)Required.GetTotalMinutes();
            const int32 TotalSeconds = (int32)Required.GetTotalSeconds() % 60;

            return FString::Printf(TEXT("TimeSince: [%s] %s, delay %dm %ds"),
                *TriggerStr, *SourceDesc, TotalMinutes, TotalSeconds);
        }

    private:
        const UChoreTimeSinceConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FChoreTimeSinceCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

// ─────────────────────────────────────────────────────────────────────────────
// EvaluateCondition — чисто state-driven, Outcome игнорируется.
// Пересчёт происходит:
//   - при любом событии через HandleEvent,
//   - и по pulse-таймеру ChoreManagerSubsystem раз в N секунд.
// ─────────────────────────────────────────────────────────────────────────────
bool UChoreTimeSinceConditionAsset::EvaluateCondition(const FOutcomeEventBase& /*Outcome*/) const
{
    UChoreManagerSubsystem* ChoreMgr = GetChoreManagerSubsystem();
    if (!ChoreMgr) return false;

    TArray<EOutcomeChore> AllowedResults;
    BuildAllowedResults(AllowedResults);
    if (AllowedResults.Num() == 0) return false;

    FDateTime LastTimestamp;
    if (!ChoreMgr->GetLatestHistoryTimestamp(
            SourceChoreId,
            Family, bUseFamily,
            Subtype, bUseSubtype,
            AllowedResults,
            LastTimestamp))
    {
        // Нет подходящего события — триггер ещё не срабатывал.
        return false;
    }

    // LastTimestamp уже в игровом времени (его записал AddHistoryEntry
    // через GetGameTime()). GetGameTime() тоже возвращает игровое время,
    // поэтому разница корректна и не «тикает», пока игра закрыта.
    const FTimespan Elapsed = ChoreMgr->GetGameTime() - LastTimestamp;
    return Elapsed >= GetRequiredDelay();
}