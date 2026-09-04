#include "MissionConditionAsset.h"
#include "ApplyMissionCompletionPolicyPayload.h"
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
            // Для всех типов (IsCompleted, IsFailed, IsAbandoned) реагируем только на MissionCompleted
            if (Outcome.OutcomeType != EOutcomeType::Mission)
                return false;
            if (Outcome.OutcomeMission != EOutcomeMission::MissionCompleted)
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