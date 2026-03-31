#include "TerminalSubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "TerminalTaskPayload.h"

void UTerminalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	CachedEventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

	if (TerminalTaskCondition)
	{
		SubscribeTerminalTask();
	}
}

void UTerminalSubsystem::Deinitialize()
{
	UnsubscribeAll();
	CachedEventBus.Reset();
	Super::Deinitialize();
}

void UTerminalSubsystem::SetTerminalTaskCondition(UOutcomeConditionAsset* NewCondition)
{
	if (TerminalTaskCondition == NewCondition) return;
	TerminalTaskCondition = NewCondition;
	if (TerminalTaskHandle.IsValid())
	{
		UnsubscribeTerminalTask();
	}
	SubscribeTerminalTask();
}

void UTerminalSubsystem::SubscribeTerminalTask()
{
	if (TerminalTaskHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("TerminalSubsystem: Already subscribed to TerminalTask"));
		return;
	}

	if (!TerminalTaskCondition)
	{
		UE_LOG(LogTemp, Warning, TEXT("TerminalSubsystem: TerminalTaskCondition is null, cannot subscribe"));
		return;
	}

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		TerminalTaskHandle = EventBus->RegisterHandler(
			TerminalTaskCondition,
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleTerminalTask)
		);

		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Subscribed to TerminalTask (handle=%u)"), TerminalTaskHandle.GetId());
	}
}

void UTerminalSubsystem::UnsubscribeTerminalTask()
{
	if (!TerminalTaskHandle.IsValid()) return;

	if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
	{
		EventBus->UnregisterHandler(TerminalTaskHandle);
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Unsubscribed TerminalTask (handle=%u)"), TerminalTaskHandle.GetId());
	}
	TerminalTaskHandle.Invalidate();
}

void UTerminalSubsystem::UnsubscribeAll()
{
	UnsubscribeTerminalTask();
}

void UTerminalSubsystem::HandleTerminalTask(const FOutcomeEventBase& Outcome)
{
	if (TerminalTaskCondition)
	{
		auto Query = TerminalTaskCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome))
		{
			UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: Incoming outcome does not satisfy TerminalTaskCondition -> ignoring"));
			return;
		}
	}

	if (UTerminalTaskPayload* P = Cast<UTerminalTaskPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Task %s - Success: %s"),
			*P->TaskId, P->bSuccess ? TEXT("true") : TEXT("false"));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("TerminalSubsystem: HandleTerminalTask called with no payload or unexpected payload type"));
	}

	OnTerminalTaskCompleted.Broadcast(Outcome);
}

