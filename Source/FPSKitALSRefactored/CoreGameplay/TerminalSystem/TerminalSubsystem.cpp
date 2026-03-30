#include "TerminalSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"

void UTerminalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
	SubscribeTerminalTask();
}

void UTerminalSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UTerminalSubsystem::SubscribeTerminalTask()
{
	if (TerminalTaskHandle.IsValid() || !TerminalTaskCondition) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		TerminalTaskHandle = EventBus->RegisterHandler(
			TerminalTaskCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleTerminalTask)
		);
	}
}

void UTerminalSubsystem::UnsubscribeTerminalTask()
{
	if (!TerminalTaskHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(TerminalTaskHandle);
	}
}

void UTerminalSubsystem::UnsubscribeAll()
{
	UnsubscribeTerminalTask();
}

void UTerminalSubsystem::HandleTerminalTask(const FOutcomeEventBase& Outcome)
{
	const FTerminalTaskOutcome& Task = static_cast<const FTerminalTaskOutcome&>(Outcome);

	UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Task %s - Success: %s"),
		*Task.TaskId, Task.bSuccess ? TEXT("true") : TEXT("false"));

	OnTerminalTaskCompleted.Broadcast(Task);
}

