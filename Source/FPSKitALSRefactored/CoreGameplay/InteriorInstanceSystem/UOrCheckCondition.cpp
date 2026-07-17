// UOrCheckCondition.cpp
#include "UOrCheckCondition.h"
#include "CheckCoordinatorComponent.h"

void UOrCheckCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Super::Initialize(InCoordinator, InEventBus);
    for (UCheckCondition* Sub : SubConditions)
    {
        if (Sub)
        {
            Sub->Initialize(InCoordinator, InEventBus);
            Sub->OnComplete.BindUObject(this, &UOrCheckCondition::OnSubConditionComplete);
        }
    }
}

void UOrCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    Reset();

    TotalValidConditions = 0;
    for (UCheckCondition* Sub : SubConditions)
        if (Sub) TotalValidConditions++;

    if (TotalValidConditions == 0)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (empty OR)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    CompletedCount = 0;
    bFinalized = false;
    bCompleted = false;
    bApproved = false;

    LogVerbose(TEXT(""), true);
    CurrentDepth++;

    for (UCheckCondition* Sub : SubConditions)
        if (Sub) Sub->ExecuteCheck(TransactionId);
}

void UOrCheckCondition::Reset()
{
    Super::Reset();
    CompletedCount = 0;
    TotalValidConditions = 0;
    bFinalized = false;
    for (UCheckCondition* Sub : SubConditions)
        if (Sub) Sub->Reset();
}

void UOrCheckCondition::OnSubConditionComplete(UCheckCondition* SubCondition)
{
    if (bFinalized || bCompleted || !SubCondition)
        return;

    CompletedCount++;
    if (SubCondition->IsApproved())
        bApproved = true;

    if (CompletedCount >= TotalValidConditions)
    {
        bFinalized = true;
        bCompleted = true;
        CurrentDepth--;
        LogVerbose(FString::Printf(TEXT("→ %s"), bApproved ? TEXT("true") : TEXT("false")), false);
        OnComplete.ExecuteIfBound(this);
    }
}

FString UOrCheckCondition::GetDescription() const
{
    TArray<FString> Parts;
    for (const UCheckCondition* Sub : SubConditions)
        Parts.Add(Sub ? Sub->GetDescription() : TEXT("Invalid"));
    return FString::Printf(TEXT("OR( %s )"), *FString::Join(Parts, TEXT(" OR ")));
}