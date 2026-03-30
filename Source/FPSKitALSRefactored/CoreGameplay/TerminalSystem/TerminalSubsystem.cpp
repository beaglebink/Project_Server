#include "TerminalSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UTerminalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	if (!EventBus || !TerminalTaskCondition) return;

	EventBus->RegisterHandler(
		TerminalTaskCondition,
		FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleTerminalTask)
	);
}

void UTerminalSubsystem::HandleTerminalTask(const FOutcomeEventBase& Outcome)
{
	const FTerminalTaskOutcome& Task = static_cast<const FTerminalTaskOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Task %s - Success: %s"),
		*Task.TaskId,
		Task.bSuccess ? TEXT("true") : TEXT("false"));

	OnTerminalTaskCompleted.Broadcast(Task);
}

