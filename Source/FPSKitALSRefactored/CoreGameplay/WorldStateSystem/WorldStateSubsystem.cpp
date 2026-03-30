#include "WorldStateSubsystem.h"
#include "MissionCompletedOutcome.h"
#include "EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeQuery.h"

void UWorldStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Handler: MissionCompleted AND Mission != Default AND Interior != Default (Обработчик: MissionCompleted И не-Default миссия И не-Default интерьер)
		auto MissionQuery = FOutcomeQueryBuilder::And();
		MissionQuery->Add(FOutcomeQueryBuilder::Type(EOutcomeType::MissionCompleted));
		MissionQuery->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Mission(EOutcomeMission::Default)));
		MissionQuery->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Interior(EOutcomeInterior::Default)));

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UWorldStateSubsystem::OnMissionCompleted),
			MissionQuery
		);

		UE_LOG(LogTemp, Warning, TEXT("WorldStateSubsystem: Registered event handler"));
	}
}

void UWorldStateSubsystem::OnMissionCompleted(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FMissionCompletedOutcome* MissionOutcome = static_cast<const FMissionCompletedOutcome*>(&Outcome);
	
	if (MissionOutcome)
	{
		// Handle completed mission specific data (Обработка специфичных данных завершённой миссии)
		UE_LOG(LogTemp, Log,
			TEXT("WorldStateSubsystem: Mission %s completed with FactKey: %s")
			TEXT(" | Mission: %d, Interior: %d"),
			*MissionOutcome->MissionId.ToString(),
			*MissionOutcome->GlobalFactKey,
			static_cast<int32>(MissionOutcome->OutcomeMission),
			static_cast<int32>(MissionOutcome->OutcomeInterior)
		);
	}
}
