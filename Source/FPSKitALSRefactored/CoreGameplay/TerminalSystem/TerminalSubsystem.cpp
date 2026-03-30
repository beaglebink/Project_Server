#include "TerminalSubsystem.h"
#include "TerminalTaskOutcome.h"
#include "EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeQuery.h"

void UTerminalSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Handler: TerminalTaskCompleted AND Object != Default (Обработчик: TerminalTaskCompleted И объект не Default)
		auto TerminalQuery = FOutcomeQueryBuilder::And();
		TerminalQuery->Add(FOutcomeQueryBuilder::Type(EOutcomeType::TerminalTaskCompleted));
		TerminalQuery->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Object(EOutcomeObject::Default)));

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::OnTerminalTaskCompleted),
			TerminalQuery
		);

		UE_LOG(LogTemp, Warning, TEXT("TerminalSubsystem: Registered event handler"));
	}
}

void UTerminalSubsystem::OnTerminalTaskCompleted(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FTerminalTaskOutcome* TerminalTaskOutcome = static_cast<const FTerminalTaskOutcome*>(&Outcome);
	
	if (TerminalTaskOutcome)
	{
		// Handle terminal task-specific data (Обработка специфичных данных терминального задания)
		UE_LOG(LogTemp, Log,
			TEXT("TerminalSubsystem: Task %s completed - Success: %s")
			TEXT(" | Object: %d"),
			*TerminalTaskOutcome->TaskId,
			TerminalTaskOutcome->bSuccess ? TEXT("True") : TEXT("False"),
			static_cast<int32>(TerminalTaskOutcome->OutcomeObject)
		);
	}
}
