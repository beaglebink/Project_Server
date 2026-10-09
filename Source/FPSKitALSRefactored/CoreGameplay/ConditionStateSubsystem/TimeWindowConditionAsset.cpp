#include "TimeWindowConditionAsset.h"
#include "ConditionStateSubsystem.h"
#include "ChoreSystem/ChoreManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

static UGameInstance* FindGameInstance()
{
    if (!GEngine) return nullptr;
    for (const FWorldContext& C : GEngine->GetWorldContexts())
    {
        UWorld* W = C.World();
        if (W && W->IsGameWorld())
            return W->GetGameInstance();
    }
    return nullptr;
}

void UTimeWindowConditionAsset::CompileCondition()
{
    class FTimeWindowCondition : public IOutcomeCondition
    {
    public:
        FTimeWindowCondition(const UTimeWindowConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (!Asset) return TEXT("TimeWindow: Invalid");

            const FString ModeStr = StaticEnum<ETimeWindowMode>()
                ->GetValueAsString(Asset->TimeMode);
            const FString ChildDesc = Asset->ChildCondition
                ? Asset->ChildCondition->GetConditionDescription()
                : TEXT("<no child>");

            return FString::Printf(TEXT("TimeWindow: [%s %.1fs] over {%s}"),
                *ModeStr, Asset->WindowSeconds, *ChildDesc);
        }
    private:
        const UTimeWindowConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FTimeWindowCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

bool UTimeWindowConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    if (!ChildCondition) return false;

    // Оборачиваемое условие должно быть скомпилировано. CompileCondition
    // идемпотентна: если уже скомпилировано — повторный вызов ничего
    // не сломает. Нужно на случай, если обёртку создали в рантайме.
    if (!ChildCondition->GetCondition().IsValid())
        ChildCondition->CompileCondition();
    if (!ChildCondition->GetCondition().IsValid()) return false;

    UGameInstance* GI = FindGameInstance();
    if (!GI) return false;

    UConditionStateSubsystem* State = GI->GetSubsystem<UConditionStateSubsystem>();
    if (!State) return false;

    const FName Key = StateKeyOverride.IsNone()
        ? FName(*GetPathName())
        : StateKeyOverride;

    // ── Шаг 1. Спрашиваем оборачиваемое условие ──────────────────────────
    const bool bChildNow = ChildCondition->GetCondition()->Evaluate(Outcome);

    if (!bChildNow)
    {
        // Внутреннее условие ложно → окно закрыто, метка сброшена.
        // Следующий подъём начнёт новое окно.
        State->ClearRiseTime(Key);
        return false;
    }

    // ── Шаг 2. Игровое время ─────────────────────────────────────────────
    // Используем ту же шкалу, что и ChoreManager, чтобы окна не «тикали»,
    // пока игра закрыта. Если ChoreManager недоступен, падаем на UtcNow —
    // лучше работать с чуть другой шкалой, чем вообще без окна.
    FDateTime Now = FDateTime::UtcNow();
    if (UChoreManagerSubsystem* CM = GI->GetSubsystem<UChoreManagerSubsystem>())
        Now = CM->GetGameTime();

    // ── Шаг 3. Фиксируем подъём, если ещё не зафиксирован ────────────────
    FDateTime LastRise = State->FindRiseTime(Key);
    if (LastRise == FDateTime::MinValue())
    {
        State->SetRiseTime(Key, Now);
        LastRise = Now;
    }

    // ── Шаг 4. Применяем окно ────────────────────────────────────────────
    const float ElapsedSeconds =
        (float)(Now - LastRise).GetTotalSeconds();

    switch (TimeMode)
    {
    case ETimeWindowMode::WithinWindow:
        return ElapsedSeconds < WindowSeconds;

    case ETimeWindowMode::AfterWindow:
        return ElapsedSeconds >= WindowSeconds;

    default:
        return false;
    }
}