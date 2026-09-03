#include "MissionConditionAsset.h"
#include "MissionEnvelopePayload.h"
#include "MissionProgressPayload.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

FName UMissionConditionAsset::GetEffectiveMissionId() const
{
    if (MissionAsset)
        return MissionAsset->GetMissionId();
    return MissionName;
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

            if (Asset->ConditionType == EMissionConditionType::IsCompleted)
            {
                if (Outcome.OutcomeMission != EOutcomeMission::MissionCompleted)
                    return false;
            }
            else if (Asset->ConditionType == EMissionConditionType::StepReached)
            {
                if (Outcome.OutcomeMission != EOutcomeMission::MissionProgress)
                    return false;
            }
            else
                return false;

            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (Asset)
            {
                FString Type = (Asset->ConditionType == EMissionConditionType::IsCompleted)
                    ? TEXT("Completed") : TEXT("Step");
                return FString::Printf(TEXT("Mission %s: %s"), 
                    *Asset->GetEffectiveMissionId().ToString(), *Type);
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

    if (ConditionType == EMissionConditionType::IsCompleted)
    {
        UMissionEnvelopePayload* Payload = Cast<UMissionEnvelopePayload>(Outcome.Payload);
        if (!Payload) return false;
        return Payload->MissionId == ExpectedId;
    }
    else if (ConditionType == EMissionConditionType::StepReached)
    {
        UMissionProgressPayload* Payload = Cast<UMissionProgressPayload>(Outcome.Payload);
        if (!Payload) return false;
        if (Payload->MissionName != ExpectedId)
            return false;

        int32 CurrentStep = Payload->StepIndex;
        switch (CompareOp)
        {
        case ECheckCompareOp::Equal:          return CurrentStep == StepIndex;
        case ECheckCompareOp::NotEqual:       return CurrentStep != StepIndex;
        case ECheckCompareOp::Greater:        return CurrentStep > StepIndex;
        case ECheckCompareOp::GreaterOrEqual: return CurrentStep >= StepIndex;
        case ECheckCompareOp::Less:           return CurrentStep < StepIndex;
        case ECheckCompareOp::LessOrEqual:    return CurrentStep <= StepIndex;
        default: return false;
        }
    }
    return false;
}