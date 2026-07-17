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
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    CompletedCount = 0;
    bFinalized = false;
    bCompleted = false;
    bApproved = false;

    for (UCheckCondition* Sub : SubConditions)
        if (Sub) Sub->ExecuteCheck(TransactionId);

    // Если все условия синхронно завершились, проверка произойдёт в OnSubConditionComplete
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
    // Если уже завершены – игнорируем
    if (bFinalized || bCompleted || !SubCondition)
        return;

    // Если подусловие одобрено – завершаем OR с true немедленно
    if (SubCondition->IsApproved())
    {
        bFinalized = true;
        bCompleted = true;
        bApproved = true;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    // Иначе увеличиваем счётчик завершённых
    CompletedCount++;

    // Если все подусловия завершены и ни одно не одобрено – false
    if (CompletedCount >= TotalValidConditions)
    {
        bFinalized = true;
        bCompleted = true;
        bApproved = false;
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