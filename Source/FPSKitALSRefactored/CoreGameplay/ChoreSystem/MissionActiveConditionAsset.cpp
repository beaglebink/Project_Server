#include "MissionActiveConditionAsset.h"
#include "MissionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

FName UMissionActiveConditionAsset::GetEffectiveMissionId() const
{
    if (MissionAsset)
        return MissionAsset->GetMissionId();
    return NAME_None;
}

FText UMissionActiveConditionAsset::GetDisplayName() const
{
    if (MissionAsset)
        return MissionAsset->DisplayName.IsEmpty() ? FText::FromString(MissionAsset->GetName()) : MissionAsset->DisplayName;
    return FText::FromString(TEXT("None"));
}

void UMissionActiveConditionAsset::CompileCondition()
{
    class FMissionActiveCondition : public IOutcomeCondition
    {
    public:
        FMissionActiveCondition(const UMissionActiveConditionAsset* InAsset) : Asset(InAsset) {}

        virtual bool Evaluate(const FOutcomeEventBase& Outcome) const override
        {
            // We ignore the event – we check the current state on each call
            // Игнорируем событие – проверяем текущее состояние при каждом вызове
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (Asset && Asset->MissionAsset)
            {
                return FString::Printf(TEXT("MissionActive: %s (Expected=%s)"),
                    *Asset->MissionAsset->GetMissionId().ToString(),
                    Asset->ExpectedActive ? TEXT("true") : TEXT("false"));
            }
            return TEXT("MissionActive: Invalid");
        }
    private:
        const UMissionActiveConditionAsset* Asset;
    };

    CompiledCondition = MakeShared<FMissionActiveCondition>(this);
    ConditionDescription = CompiledCondition->Describe();
}

bool UMissionActiveConditionAsset::EvaluateCondition(const FOutcomeEventBase& Outcome) const
{
    FName MissionId = GetEffectiveMissionId();
    if (MissionId.IsNone())
        return false;

    // Get the MissionSubsystem
    // Получаем MissionSubsystem
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
        return false;

    // Check mission activity
    // Проверяем активность миссии
    bool bIsActive = MissionSub->IsMissionActive(MissionId);
    return bIsActive == ExpectedActive;
}