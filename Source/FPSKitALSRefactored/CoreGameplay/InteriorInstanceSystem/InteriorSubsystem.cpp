#include "InteriorSubsystem.h"
#include "EventBusSubsystem.h"
#include "InteriorTransitionOutcome.h"
#include "Outcome.h"

void UInteriorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &UInteriorSubsystem::HandleOutcome);
    }
}

void UInteriorSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    switch (Outcome.OutcomeType)
    {
        case EOutcomeType::GhostCleared:
        {
            const FInteriorTransitionOutcome* ItemAcquiredOutcome = static_cast<const FInteriorTransitionOutcome*>(&Outcome);
            if (ItemAcquiredOutcome)
            {
                UE_LOG(LogTemp, Log, TEXT("InteriorSubsystem: Item transitioned, updating interior state. FloorIndex %i"), ItemAcquiredOutcome->FloorIndex);
            }
            break;
        }
        default:
            break;
    }
}
