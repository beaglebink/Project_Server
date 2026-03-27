#include "MissionSubsystem.h"
#include "MissionProgressOutcome.h"
#include "EventBusSubsystem.h"

void UMissionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &UMissionSubsystem::HandleOutcome);
    }
}

void UMissionSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == "GhostCleared")
    {
        const FMissionProgressOutcome* MissionProgress = static_cast<const FMissionProgressOutcome*>(&Outcome);
        if (MissionProgress)
        {
            UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost encounter cleared, advancing mission step. MissionName: %s"), *MissionProgress->MissionName);
        }
    }
}
