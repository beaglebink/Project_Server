// InteriorObjectCheckCondition.cpp
#include "InteriorObjectCheckCondition.h"
#include "FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

// ----- UInteriorObjectExistsCondition -----
void UInteriorObjectExistsCondition::ExecuteCheck(const FGuid& TransactionId)
{
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bool bFound = false;

    AActor* Actor = ActorRef.LoadSynchronous();
    if (IsValid(Actor))
    {
        // Проверяем, что актор находится в текущем мире и имеет компонент FloorAssignment (опционально)
        // Можно просто проверить, что актор не помечен на удаление и находится в мире
        // Для надёжности проверяем, что актор действительно принадлежит этому миру
        if (Actor->GetWorld() == World && IsValid(Actor))
        {
            // Если нужна проверка наличия компонента FloorAssignment, можно раскомментировать:
            // UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
            // if (Comp)
            //     bFound = true;
            // Но проще считать, что любой актор, на который есть ссылка и он валиден – существует
            bFound = true;
        }
    }

    bApproved = (bFound == bShouldExist);
    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorObjectExistsCondition::GetDescription() const
{
    FString ActorName = ActorRef.IsValid() ? ActorRef->GetName() : ActorRef.ToString();
    return FString::Printf(TEXT("Object %s exists: %s"), *ActorName, bShouldExist ? TEXT("true") : TEXT("false"));
}

// ----- UInteriorObjectDestroyedCondition -----
void UInteriorObjectDestroyedCondition::ExecuteCheck(const FGuid& TransactionId)
{
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bool bExists = false;

    AActor* Actor = ActorRef.LoadSynchronous();
    if (IsValid(Actor) && Actor->GetWorld() == World)
    {
        bExists = true;
    }

    bool bIsDestroyed = !bExists;
    bApproved = (bIsDestroyed == bShouldBeDestroyed);
    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorObjectDestroyedCondition::GetDescription() const
{
    FString ActorName = ActorRef.IsValid() ? ActorRef->GetName() : ActorRef.ToString();
    return FString::Printf(TEXT("Object %s destroyed: %s"), *ActorName, bShouldBeDestroyed ? TEXT("true") : TEXT("false"));
}