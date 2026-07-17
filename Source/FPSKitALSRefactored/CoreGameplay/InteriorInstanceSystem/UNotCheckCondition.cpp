// UNotCheckCondition.cpp
#include "UNotCheckCondition.h"
#include "CheckCoordinatorComponent.h"

void UNotCheckCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Super::Initialize(InCoordinator, InEventBus);
    if (InnerCondition)
    {
        InnerCondition->Initialize(InCoordinator, InEventBus);
        InnerCondition->OnComplete.BindUObject(this, &UNotCheckCondition::OnInnerConditionComplete);
    }
}

void UNotCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    Reset();

    if (!InnerCondition)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no inner)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;
    bFinalized = false;

    LogVerbose(TEXT(""), true);
    CurrentDepth++;

    InnerCondition->ExecuteCheck(TransactionId);
}

void UNotCheckCondition::Reset()
{
    Super::Reset();
    bFinalized = false;
    if (InnerCondition)
        InnerCondition->Reset();
}

void UNotCheckCondition::OnInnerConditionComplete(UCheckCondition* Condition)
{
    if (bCompleted || bFinalized)
        return;

    bFinalized = true;
    bCompleted = true;
    bApproved = !Condition->IsApproved();

    CurrentDepth--;
    LogVerbose(FString::Printf(TEXT("→ %s"), bApproved ? TEXT("true") : TEXT("false")), false);
    OnComplete.ExecuteIfBound(this);
}

FString UNotCheckCondition::GetDescription() const
{
    if (InnerCondition)
        return FString::Printf(TEXT("NOT( %s )"), *InnerCondition->GetDescription());
    else
        return TEXT("NOT( Invalid )");
}