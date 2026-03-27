#include "MissionSubsystem.h"
#include "GhostClearedOutcome.h"
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
        const FGhostClearedOutcome* GhostOutcome = static_cast<const FGhostClearedOutcome*>(&Outcome);
        if (GhostOutcome)
        {

            UE_LOG(LogTemp, Log, TEXT("MissionSubsystem: Ghost encounter cleared, advancing mission step."));
        }
    }
}
