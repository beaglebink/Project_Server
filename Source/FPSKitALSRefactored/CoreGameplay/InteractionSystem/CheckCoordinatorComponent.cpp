// CheckCoordinatorComponent.cpp
#include "CheckCoordinatorComponent.h"
#include "CheckCondition.h"
#include "Engine/World.h"
#include "TimerManager.h"

UCheckCoordinatorComponent::UCheckCoordinatorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCheckCoordinatorComponent::BeginPlay()
{
    Super::BeginPlay();
    EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!EventBus)
    {
        UE_LOG(LogTemp, Error, TEXT("CheckCoordinator: EventBus not found!"));
        return;
    }

    for (UCheckCondition* Cond : Conditions)
    {
        if (Cond)
        {
            Cond->Initialize(this, EventBus);
            Cond->OnComplete.BindUObject(this, &UCheckCoordinatorComponent::OnConditionComplete);
        }
    }
}

void UCheckCoordinatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    /*
    for (auto& Pair : ActiveChecks)
    {
        GetWorld()->GetTimerManager().ClearTimer(Pair.Value.GlobalTimeoutTimer);
    }
    */
    ActiveChecks.Empty();
    Super::EndPlay(EndPlayReason);
}

void UCheckCoordinatorComponent::StartCheck()
{
    if (!EventBus)
    {
        UE_LOG(LogTemp, Error, TEXT("CheckCoordinator: Cannot start check, EventBus missing."));
        return;
    }

    TArray<UCheckCondition*> ValidConditions;
    for (UCheckCondition* Cond : Conditions)
    {
        if (Cond) ValidConditions.Add(Cond);
    }

    if (ValidConditions.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("CheckCoordinator: No valid conditions, approving immediately."));
        OnAllApproved.Broadcast();
        return;
    }

    FGuid Txn = FGuid::NewGuid();
    FPendingCheck& Check = ActiveChecks.Add(Txn);
    Check.TransactionId = Txn;
    Check.TotalConditions = ValidConditions.Num();
    Check.CompletedCount = 0;
    Check.bFinalized = false;

    for (UCheckCondition* Cond : ValidConditions)
    {
        Cond->ExecuteCheck(Txn);
    }
    /*
    GetWorld()->GetTimerManager().SetTimer(Check.GlobalTimeoutTimer,
        FTimerDelegate::CreateUObject(this, &UCheckCoordinatorComponent::OnTimeout, Txn),
        GlobalTimeoutSeconds, false);
    */
    UE_LOG(LogTemp, Log, TEXT("CheckCoordinator: Started check with Txn=%s, %d conditions."), *Txn.ToString(), ValidConditions.Num());
}

void UCheckCoordinatorComponent::OnConditionComplete(UCheckCondition* Condition)
{
    if (!Condition) return;
    FGuid Txn = Condition->GetCurrentTransactionId();
    FPendingCheck* Check = ActiveChecks.Find(Txn);
    if (!Check || Check->bFinalized) return;

    Check->CompletedCount++;

    if (!Condition->IsApproved())
    {
        FinalizeCheck(Txn, false);
        return;
    }

    if (Check->CompletedCount >= Check->TotalConditions)
    {
        bool bAllApproved = true;
        for (UCheckCondition* Cond : Conditions)
        {
            if (Cond && !Cond->IsApproved())
            {
                bAllApproved = false;
                break;
            }
        }
        FinalizeCheck(Txn, bAllApproved);
    }
}

void UCheckCoordinatorComponent::FinalizeCheck(const FGuid& TransactionId, bool bSuccess)
{
    FPendingCheck* Check = ActiveChecks.Find(TransactionId);
    if (!Check || Check->bFinalized) return;

    Check->bFinalized = true;
    //GetWorld()->GetTimerManager().ClearTimer(Check->GlobalTimeoutTimer);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("CheckCoordinator: All conditions approved! Txn=%s"), *TransactionId.ToString());
        OnAllApproved.Broadcast();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("CheckCoordinator: Check rejected. Txn=%s"), *TransactionId.ToString());
        OnAnyRejected.Broadcast();
    }

    ActiveChecks.Remove(TransactionId);
}
/*
void UCheckCoordinatorComponent::OnTimeout(FGuid TransactionId)
{
    FPendingCheck* Check = ActiveChecks.Find(TransactionId);
    if (!Check || Check->bFinalized) return;

    UE_LOG(LogTemp, Error, TEXT("CheckCoordinator: Global timeout for Txn=%s, %d conditions remaining."),
        *TransactionId.ToString(), Check->TotalConditions - Check->CompletedCount);
    FinalizeCheck(TransactionId, false);
}
*/