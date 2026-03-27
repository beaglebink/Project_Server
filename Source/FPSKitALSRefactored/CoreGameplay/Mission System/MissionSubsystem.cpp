#include "MissionSubsystem.h"
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
        UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost encounter cleared, advancing mission step."));
    }
}
