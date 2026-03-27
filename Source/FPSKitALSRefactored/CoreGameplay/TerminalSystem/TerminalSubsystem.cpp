#include "TerminalSubsystem.h"
#include "EventBusSubsystem.h"

void UTerminalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UTerminalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

    if (UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &UTerminalSubsystem::HandleOutcome);
    }
}

void UTerminalSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == "TerminalTaskCompleted")
    {
        UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Terminal task completed, updating terminal data."));
    }
}
