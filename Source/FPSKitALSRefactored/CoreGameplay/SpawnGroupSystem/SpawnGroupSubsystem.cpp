#include "SpawnGroupSubsystem.h"
#include "GhostClearedOutcome.h"
#include "EventBusSubsystem.h"

void USpawnGroupSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void USpawnGroupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (UEventBusSubsystem* EventBus = InWorld.GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
    {
        EventBus->OnOutcomeEvent.AddDynamic(this, &USpawnGroupSubsystem::HandleOutcome);
    }
}

void USpawnGroupSubsystem::HandleOutcome(const FOutcomeEventBase& Outcome)
{
    switch (Outcome.OutcomeType)
    {
        case EOutcomeType::GhostCleared:
        {
            const FGhostClearedOutcome* GhostCleared = static_cast<const FGhostClearedOutcome*>(&Outcome);
            if (GhostCleared)
            {
                UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Ghost encounter cleared, resetting spawn group. GhostType: %s"), *GhostCleared->GhostType);
            }
            break;
        }
        default:
            break;
    }
}
