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
    // Сбрасываем состояние
    Reset();

    // Если нет подчинённого условия – считаем условие невыполненным (false)
    if (!InnerCondition)
    {
        bCompleted = true;
        bApproved = false;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    // Запускаем подчинённое условие
    InnerCondition->ExecuteCheck(TransactionId);
}

void UNotCheckCondition::Reset()
{
    Super::Reset();
    if (InnerCondition)
        InnerCondition->Reset();
}

void UNotCheckCondition::OnInnerConditionComplete(UCheckCondition* Condition)
{
    if (bCompleted)
        return;

    bCompleted = true;
    // Инвертируем результат подчинённого условия
    bApproved = !Condition->IsApproved();

    OnComplete.ExecuteIfBound(this);
}

FString UNotCheckCondition::GetDescription() const
{
    if (InnerCondition)
        return FString::Printf(TEXT("NOT( %s )"), *InnerCondition->GetDescription());
    else
        return TEXT("NOT( Invalid )");
}