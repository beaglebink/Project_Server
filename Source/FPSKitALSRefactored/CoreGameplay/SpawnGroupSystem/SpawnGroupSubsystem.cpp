#include "SpawnGroupSubsystem.h"
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
    if (Outcome.OutcomeType == "GhostCleared")
    {
        UE_LOG(LogTemp, Log, TEXT("SpawnGroupSubsystem: Ghost encounter cleared, resetting spawn group."));
    }
}
