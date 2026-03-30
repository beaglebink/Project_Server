#include "InteriorSubsystem.h"
#include "EventBusSubsystem.h"
#include "InteriorTransitionOutcome.h"
#include "Outcome.h"
#include "../EventBusSystem/OutcomeQuery.h"

void UInteriorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	if (UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Handler 1: GhostCleared AND Interior != Default (Обработчик 1: GhostCleared И интерьер не Default)
		auto GhostWithInterior = FOutcomeQueryBuilder::And();
		GhostWithInterior->Add(FOutcomeQueryBuilder::Type(EOutcomeType::GhostCleared));
		GhostWithInterior->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Interior(EOutcomeInterior::Default)));

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::OnGhostClearedWithInterior),
			GhostWithInterior
		);

		// Handler 2: ItemAcquired AND Object != Default (Обработчик 2: ItemAcquired И объект не Default)
		auto ItemWithObject = FOutcomeQueryBuilder::And();
		ItemWithObject->Add(FOutcomeQueryBuilder::Type(EOutcomeType::ItemAcquired));
		ItemWithObject->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Object(EOutcomeObject::Default)));

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UInteriorSubsystem::OnItemAcquiredWithObject),
			ItemWithObject
		);

		UE_LOG(LogTemp, Warning, TEXT("InteriorSubsystem: Registered 2 event handlers"));
	}
}

void UInteriorSubsystem::OnGhostClearedWithInterior(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FInteriorTransitionOutcome* TransitionOutcome = static_cast<const FInteriorTransitionOutcome*>(&Outcome);
	
	if (TransitionOutcome)
	{
		// Handle ghost-specific data when cleared in interior (Обработка специфичных данных при очищении привидения в интерьере)
		UE_LOG(LogTemp, Log,
			TEXT("InteriorSubsystem: Ghost cleared - updating floor %d")
			TEXT(" | Interior: %d"),
			TransitionOutcome->FloorIndex,
			static_cast<int32>(TransitionOutcome->OutcomeInterior)
		);
	}
}

void UInteriorSubsystem::OnItemAcquiredWithObject(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FInteriorTransitionOutcome* ItemOutcome = static_cast<const FInteriorTransitionOutcome*>(&Outcome);
	
	if (ItemOutcome)
	{
		// Handle item-specific data when acquired in interior (Обработка специфичных данных при получении предмета в интерьере)
		UE_LOG(LogTemp, Log,
			TEXT("InteriorSubsystem: Item acquired - updating interior state at floor %d")
			TEXT(" | Object: %d"),
			ItemOutcome->FloorIndex,
			static_cast<int32>(ItemOutcome->OutcomeObject)
		);
	}
}
