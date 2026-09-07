// MissionConditionAsset.cpp
#include "MissionConditionAsset.h"
#include "ApplyMissionCompletionPolicyPayload.h"
#include "MissionEnvelopePayload.h"
#include "MissionProgressPayload.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

static UMissionSubsystem* GetMissionSubsystem()
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
                return GI->GetSubsystem<UMissionSubsystem>();
            }
        }
    }
    return nullptr;

}
FName UMissionConditionAsset::GetEffectiveMissionId() const
{
    if (MissionAsset)
        return MissionAsset->GetMissionId();
    return NAME_None;
}

FText UMissionConditionAsset::GetDisplayName() const
{
    if (MissionAsset)
    {
        return MissionAsset->DisplayName.IsEmpty()
            ? FText::FromString(MissionAsset->GetName())
            : MissionAsset->DisplayName;
    }
    return FText::FromString(TEXT("None"));
}

void UMissionConditionAsset::CompileCondition()
{
    class FMissionCondition : public IOutcomeCondition
    {
    public:
        FMissionCondition(const UMissionConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            if (Outcome.OutcomeType != EOutcomeType::Mission)
                return false;

            // Для StepReached не фильтруем по подтипу, чтобы проверять при любом событии Mission
            // Для остальных – фильтруем по подтипу
            switch (Asset->ConditionType)
            {
            case EMissionConditionType::IsCompleted:
            case EMissionConditionType::IsFailed:
            case EMissionConditionType::IsAbandoned:
                if (Outcome.OutcomeMission != EOutcomeMission::MissionCompleted)
                    return false;
                break;
            case EMissionConditionType::StepReached:
                // Пропускаем любые Mission-события
                break;
            default:
                return false;
            }

            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (Asset)
            {
                FString TypeName;
                switch (Asset->ConditionType)
                {
                case EMissionConditionType::IsCompleted:   TypeName = TEXT("Completed"); break;
                case EMissionConditionType::IsFailed:      TypeName = TEXT("Failed"); break;
                case EMissionConditionType::IsAbandoned:   TypeName = TEXT("Abandoned"); break;
                case EMissionConditionType::StepReached:   TypeName = FString::Printf(TEXT("Step %d"), Asset->StepIndex); break;
                default: TypeName = TEXT("Unknown");
                }
                return FString::Printf(TEXT("Mission %s: %s"),
                    *Asset->GetDisplayName().ToString(), *TypeName);
            }
            return TEXT("Invalid");
        }

    private:
        const UMissionConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FMissionCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

bool UMissionConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    FName ExpectedId = GetEffectiveMissionId();
    if (ExpectedId.IsNone())
        return false;

    // Для завершения миссии (Completed, Failed, Abandoned)
    if (ConditionType == EMissionConditionType::IsCompleted ||
        ConditionType == EMissionConditionType::IsFailed ||
        ConditionType == EMissionConditionType::IsAbandoned)
    {
        UMissionSubsystem* MissionSub = GetMissionSubsystem();
        if (!MissionSub) return false;
        if (MissionSub->IsMissionActive(ExpectedId))
            return false;

        UApplyMissionCompletionPolicyPayload* Payload = Cast<UApplyMissionCompletionPolicyPayload>(Outcome.Payload);
        if (!Payload) return false;
        if (Payload->MissionId != ExpectedId) return false;

        EMissionEndReason requiredReason;
        switch (ConditionType)
        {
        case EMissionConditionType::IsCompleted: requiredReason = EMissionEndReason::Completed; break;
        case EMissionConditionType::IsFailed:    requiredReason = EMissionEndReason::Failed; break;
        case EMissionConditionType::IsAbandoned: requiredReason = EMissionEndReason::Abandoned; break;
        default: return false;
        }
        return Payload->EndReason == requiredReason;
    }

    // Для проверки шага (StepReached)
    if (ConditionType == EMissionConditionType::StepReached)
    {
        UMissionSubsystem* MissionSub = nullptr;
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
                        MissionSub = GI->GetSubsystem<UMissionSubsystem>();
                        if (MissionSub) break;
                    }
                }
            }
        }
        if (!MissionSub)
        {
            UE_LOG(LogTemp, Warning, TEXT("MissionConditionAsset::EvaluateCondition: MissionSubsystem not found for StepReached"));
            return false;
        }

        int32 CurrentStep = MissionSub->GetMissionStep(ExpectedId);
        UE_LOG(LogTemp, Log, TEXT("MissionConditionAsset::EvaluateCondition: StepReached, MissionId=%s, CurrentStep=%d, ExpectedStep=%d, CompareOp=%d"),
            *ExpectedId.ToString(), CurrentStep, StepIndex, (int32)CompareOp);

        if (CurrentStep < 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("MissionConditionAsset::EvaluateCondition: Mission '%s' not found in ActiveMissions"), *ExpectedId.ToString());
            return false;
        }

        switch (CompareOp)
        {
        case ECheckCompareOp::Equal:          return CurrentStep == StepIndex;
        case ECheckCompareOp::NotEqual:       return CurrentStep != StepIndex;
        case ECheckCompareOp::Less:           return CurrentStep < StepIndex;
        case ECheckCompareOp::LessOrEqual:    return CurrentStep <= StepIndex;
        case ECheckCompareOp::Greater:        return CurrentStep > StepIndex;
        case ECheckCompareOp::GreaterOrEqual: return CurrentStep >= StepIndex;
        default: return false;
        }
    }

    return false;
}