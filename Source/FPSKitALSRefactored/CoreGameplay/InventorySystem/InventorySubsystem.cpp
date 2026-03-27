#include "InventorySubsystem.h"
#include "EventBusSubsystem.h"
#include "ItemAcquiredOutcome.h"

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (UEventBusSubsystem* EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &UInventorySubsystem::HandleOutcome);
    }
}

void UInventorySubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == "ItemAcquired")
    {
        const FItemAcquiredOutcome* ItemOutcome = static_cast<const FItemAcquiredOutcome*>(&Outcome);
        if (ItemOutcome)
        {
            UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired with ObjectId %s"), *ItemOutcome->ObjectId.ToString());
        }
    }
}
