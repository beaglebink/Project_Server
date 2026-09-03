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
            // Игнорируем событие – проверяем текущее состояние при каждом вызове
            return Asset ? Asset->EvaluateCondition(Outcome) : false;
        }

        virtual FString Describe() const override
        {
            if (Asset)
            {
                return FString::Printf(TEXT("Mission %s is %s"),
                    *Asset->GetDisplayName().ToString(),
                    Asset->ExpectedActive ? TEXT("Active") : TEXT("Not Active"));
            }
            return TEXT("Invalid");
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

    // Проверяем активность миссии
    bool bIsActive = MissionSub->IsMissionActive(MissionId);
    return bIsActive == ExpectedActive;
}