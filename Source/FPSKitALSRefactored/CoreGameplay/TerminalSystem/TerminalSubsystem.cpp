#include "TerminalSubsystem.h"
#include "TerminalTaskOutcome.h"
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
    switch (Outcome.OutcomeType)
    {
        case EOutcomeType::TerminalTaskCompleted:
        {
            const FTerminalTaskOutcome* TerminalTaskOutcome = static_cast<const FTerminalTaskOutcome*>(&Outcome);
            if (TerminalTaskOutcome)
            {
                UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Terminal task completed. TerminalId: %s, TaskId: %s, Success: %s"), *TerminalTaskOutcome->TerminalId.ToString(), *TerminalTaskOutcome->TaskId, TerminalTaskOutcome->bSuccess ? TEXT("True") : TEXT("False"));
            }
            break;
        }
        default:
            break;
    }
}
