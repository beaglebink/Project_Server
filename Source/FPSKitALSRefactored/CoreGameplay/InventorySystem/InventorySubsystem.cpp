#include "InventorySubsystem.h"
#include "EventBusSubsystem.h"
#include "ItemAcquiredOutcome.h"
#include "Outcome.h"
#include "../EventBusSystem/OutcomeQuery.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
	{
		// Handler: ItemAcquired AND (Object != Default OR Interior != Default) (Обработчик: ItemAcquired И (объект не Default ИЛИ интерьер не Default))
		auto InventoryQuery = FOutcomeQueryBuilder::And();
		InventoryQuery->Add(FOutcomeQueryBuilder::Type(EOutcomeType::ItemAcquired));
		
		auto ObjectOrInterior = FOutcomeQueryBuilder::Or();
		ObjectOrInterior->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Object(EOutcomeObject::Default)));
		ObjectOrInterior->Add(FOutcomeQueryBuilder::Not(FOutcomeQueryBuilder::Interior(EOutcomeInterior::Default)));
		InventoryQuery->Add(ObjectOrInterior);

		EventBus->RegisterHandler(
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::OnItemAcquired),
			InventoryQuery
		);

		UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem: Registered event handler"));
	}
}

void UInventorySubsystem::OnItemAcquired(const FOutcomeEventBase& Outcome)
{
	// Cast to specialized type (Кастируем в специализированный тип)
	const FItemAcquiredOutcome* ItemOutcome = static_cast<const FItemAcquiredOutcome*>(&Outcome);
	
	if (ItemOutcome)
	{
		// Handle item-specific data (Обработка специфичных данных предмета)
		UE_LOG(LogTemp, Log,
			TEXT("InventorySubsystem: Item %s acquired")
			TEXT(" | Object: %d, Interior: %d"),
			*ItemOutcome->ObjectId.ToString(),
			static_cast<int32>(ItemOutcome->OutcomeObject),
			static_cast<int32>(ItemOutcome->OutcomeInterior)
		);
	}
}
