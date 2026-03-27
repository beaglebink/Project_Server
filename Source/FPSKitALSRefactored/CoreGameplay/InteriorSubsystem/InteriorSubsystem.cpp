#include "InteriorSubsystem.h"
#include "EventBusSubsystem.h"

void UInteriorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    //if (UEventBusSubsystem* EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    //{
    //    EventBus->OnOutcomeEvent.AddDynamic(this, &UInteriorSubsystem::HandleOutcome);
    //}
}

void UInteriorSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == "GhostCleared")
    {
        UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Ghost cleared, updating interior state."));
    }
}
