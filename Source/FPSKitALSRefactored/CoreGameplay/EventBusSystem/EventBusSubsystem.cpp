#include "EventBusSubsystem.h"

void UEventBusSubsystem::PublishOutcome(const FOutcomeEventBase& Outcome)
{
	// Call all handlers whose conditions are met (Вызываем все обработчики, чьи условия выполнены)
	for (const FOutcomeHandlerEntry& Entry : Handlers)
	{
		// If condition is not set or condition is met - call the handler (Если условие не задано или условие выполнено - вызываем обработчик)
		if (!Entry.Query.IsValid() || Entry.Query->Evaluate(Outcome))
		{
			Entry.Handler.ExecuteIfBound(Outcome);
		}
	}

	// Also broadcast through delegate for Blueprint compatibility (Также отправляем через делегат для Blueprint)
	OnOutcomeEvent.Broadcast(Outcome);
}
