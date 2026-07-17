// InteriorObjectCheckCondition.cpp
#include "InteriorObjectCheckCondition.h"
#include "FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

// ----- UInteriorObjectExistsCondition -----
void UInteriorObjectExistsCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no world)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bool bFound = false;
    AActor* Actor = ActorRef.Get();
    if (IsValid(Actor) && Actor->GetWorld() == World)
        bFound = true;

    bApproved = (bFound == bShouldExist);
    bCompleted = true;

    // Сначала закрывающая строка, потом OnComplete
    LogVerbose(FString::Printf(TEXT("→ %s"), bApproved ? TEXT("true") : TEXT("false")), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorObjectExistsCondition::GetDescription() const
{
    FString ActorName = ActorRef.IsValid() ? ActorRef->GetName() : TEXT("None");
    return FString::Printf(TEXT("Object %s exists: %s"), *ActorName, bShouldExist ? TEXT("true") : TEXT("false"));
}

// ----- UInteriorObjectDestroyedCondition -----
void UInteriorObjectDestroyedCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no world)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bool bExists = false;
    AActor* Actor = ActorRef.Get();
    if (IsValid(Actor) && Actor->GetWorld() == World)
        bExists = true;

    bool bIsDestroyed = !bExists;
    bApproved = (bIsDestroyed == bShouldBeDestroyed);
    bCompleted = true;

    LogVerbose(FString::Printf(TEXT("→ %s"), bApproved ? TEXT("true") : TEXT("false")), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorObjectDestroyedCondition::GetDescription() const
{
    FString ActorName = ActorRef.IsValid() ? ActorRef->GetName() : TEXT("None");
    return FString::Printf(TEXT("Object %s destroyed: %s"), *ActorName, bShouldBeDestroyed ? TEXT("true") : TEXT("false"));
}