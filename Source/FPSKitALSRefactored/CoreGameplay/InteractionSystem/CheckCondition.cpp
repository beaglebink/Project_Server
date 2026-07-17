// CheckCondition.cpp
#include "CheckCondition.h"
#include "CheckCoordinatorComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UCheckCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Coordinator = InCoordinator;
    EventBus = InEventBus;

    if (EventBus)
    {
        UOutcomeConditionAsset* Asset = CreateSubscriptionCondition();
        if (Asset)
        {
            EventBus->RegisterHandler(Asset, FOutcomeHandlerDelegate::CreateUObject(this, &UCheckCondition::OnCheckResponse));
        }
    }
}
/*
void UCheckCondition::StartTimeoutTimer(const FGuid& TransactionId)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(TimeoutTimerHandle,
            FTimerDelegate::CreateUObject(this, &UCheckCondition::OnTimeout, TransactionId),
            TimeoutSeconds, false);
    }
}

void UCheckCondition::ClearTimeoutTimer()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimeoutTimerHandle);
    }
}

void UCheckCondition::OnTimeout(FGuid TransactionId)
{
    if (!bCompleted && CurrentTransactionId == TransactionId)
    {
        bCompleted = true;
        bApproved = false;
        UE_LOG(LogTemp, Warning, TEXT("UCheckCondition: Timeout for Txn %s"), *TransactionId.ToString());
        OnComplete.ExecuteIfBound(this);
    }
}
*/
UWorld* UCheckCondition::GetWorld() const
{
    if (Coordinator)
    {
        return Coordinator->GetWorld();
    }
    return nullptr;
}

void UCheckCondition::Reset()
{
    bCompleted = false;
    bApproved = false;
    CurrentTransactionId.Invalidate();
    //ClearTimeoutTimer();
}