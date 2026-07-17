// UAndCheckCondition.cpp
#include "UAndCheckCondition.h"
#include "CheckCoordinatorComponent.h"

void UAndCheckCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Super::Initialize(InCoordinator, InEventBus);
    for (UCheckCondition* Sub : SubConditions)
    {
        if (Sub)
        {
            Sub->Initialize(InCoordinator, InEventBus);
            Sub->OnComplete.BindUObject(this, &UAndCheckCondition::OnSubConditionComplete);
        }
    }
}

void UAndCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    Reset();

    TotalValidConditions = 0;
    for (UCheckCondition* Sub : SubConditions)
        if (Sub) TotalValidConditions++;

    if (TotalValidConditions == 0)
    {
        bCompleted = true;
        bApproved = true;
        LogVerbose(TEXT("→ true (empty AND)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    CompletedCount = 0;
    bFinalized = false;
    bCompleted = false;
    bApproved = true;

    // Открывающая строка – с текущей глубиной (уровень родителя)
    LogVerbose(TEXT(""), true);
    // Затем увеличиваем глубину для внутренних элементов
    CurrentDepth++;

    for (UCheckCondition* Sub : SubConditions)
        if (Sub) Sub->ExecuteCheck(TransactionId);
}

void UAndCheckCondition::Reset()
{
    Super::Reset();
    CompletedCount = 0;
    TotalValidConditions = 0;
    bFinalized = false;
    for (UCheckCondition* Sub : SubConditions)
        if (Sub) Sub->Reset();
}

void UAndCheckCondition::OnSubConditionComplete(UCheckCondition* SubCondition)
{
    if (!SubCondition || bFinalized || bCompleted)
        return;

    CompletedCount++;
    if (!SubCondition->IsApproved())
        bApproved = false;

    if (CompletedCount >= TotalValidConditions)
    {
        bFinalized = true;
        bCompleted = true;
        // Возвращаем глубину к уровню родителя
        CurrentDepth--;
        // Закрывающая строка – на том же уровне, что и открывающая
        LogVerbose(FString::Printf(TEXT("→ %s"), bApproved ? TEXT("true") : TEXT("false")), false);
        OnComplete.ExecuteIfBound(this);
    }
}

FString UAndCheckCondition::GetDescription() const
{
    TArray<FString> Parts;
    for (const UCheckCondition* Sub : SubConditions)
        Parts.Add(Sub ? Sub->GetDescription() : TEXT("Invalid"));
    return FString::Printf(TEXT("AND( %s )"), *FString::Join(Parts, TEXT(" AND ")));
}