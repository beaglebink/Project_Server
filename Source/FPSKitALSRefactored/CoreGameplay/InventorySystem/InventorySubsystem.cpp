#include "InventorySubsystem.h"
#include "EventBusSubsystem.h"
#include "ItemAcquiredOutcome.h"
#include "Outcome.h"

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
    switch (Outcome.OutcomeType)
    {
        case EOutcomeType::ItemAcquired:
        {
            const FItemAcquiredOutcome* ItemOutcome = static_cast<const FItemAcquiredOutcome*>(&Outcome);
            if (ItemOutcome)
            {
                UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired with ObjectId %s"), *ItemOutcome->ObjectId.ToString());
            }
            break;
        }
        default:
            break;
    }
}
