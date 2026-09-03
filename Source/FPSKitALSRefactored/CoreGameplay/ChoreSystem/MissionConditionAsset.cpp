// MissionConditionAsset.cpp
#include "MissionConditionAsset.h"
#include "ApplyMissionCompletionPolicyPayload.h"
#include "MissionEnvelopePayload.h"
#include "MissionProgressPayload.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

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

            EOutcomeMission requiredMissionType = EOutcomeMission::Default;
            switch (Asset->ConditionType)
            {
            case EMissionConditionType::IsCompleted:
            case EMissionConditionType::IsFailed:
            case EMissionConditionType::IsAbandoned:
                requiredMissionType = EOutcomeMission::MissionCompleted;
                break;
            case EMissionConditionType::IsActivated:
                requiredMissionType = EOutcomeMission::MissionActivated;
                break;
            default:
                return false;
            }

            if (Outcome.OutcomeMission != requiredMissionType)
                return false;

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
                case EMissionConditionType::IsActivated:   TypeName = TEXT("Activated"); break;
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

    switch (ConditionType)
    {
    case EMissionConditionType::IsCompleted:
    case EMissionConditionType::IsFailed:
    case EMissionConditionType::IsAbandoned:
    {
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

    case EMissionConditionType::IsActivated:
    {
        UMissionEnvelopePayload* Payload = Cast<UMissionEnvelopePayload>(Outcome.Payload);
        if (!Payload) return false;
        return Payload->MissionId == ExpectedId;
    }

    default:
        return false;
    }
}