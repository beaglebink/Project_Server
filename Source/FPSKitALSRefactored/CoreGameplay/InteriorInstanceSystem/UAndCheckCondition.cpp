// UAndCheckCondition.cpp
#include "UAndCheckCondition.h"
#include "CheckCoordinatorComponent.h"

void UAndCheckCondition::Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus)
{
    Super::Initialize(InCoordinator, InEventBus);

    // Инициализируем каждое подчинённое условие и подписываемся на его OnComplete
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
    // Сбрасываем состояние
    Reset();

    // Подсчитываем количество валидных условий
    TotalValidConditions = 0;
    for (UCheckCondition* Sub : SubConditions)
    {
        if (Sub) TotalValidConditions++;
    }

    // Если нет валидных условий – считаем условие выполненным (true) (пустое И)
    if (TotalValidConditions == 0)
    {
        bCompleted = true;
        bApproved = true;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    CurrentTransactionId = TransactionId;
    CompletedCount = 0;
    bFinalized = false;
    bCompleted = false;
    bApproved = false;

    // Запускаем все подчинённые условия
    for (UCheckCondition* Sub : SubConditions)
    {
        if (Sub)
        {
            Sub->ExecuteCheck(TransactionId);
        }
        // Невалидные условия не учитываем – они уже исключены из TotalValidConditions
    }

    // Если все подчинённые условия уже синхронно завершились (было обработано через OnSubConditionComplete),
    // проверка будет выполнена там. Если же все ещё не завершены – ждём.
}

void UAndCheckCondition::Reset()
{
    Super::Reset();
    CompletedCount = 0;
    TotalValidConditions = 0;
    bFinalized = false;

    // Сбрасываем все подчинённые условия
    for (UCheckCondition* Sub : SubConditions)
    {
        if (Sub)
        {
            Sub->Reset();
        }
    }
}

void UAndCheckCondition::OnSubConditionComplete(UCheckCondition* SubCondition)
{
    // Если уже завершены или условие невалидно – игнорируем
    if (!SubCondition || bFinalized || bCompleted)
        return;

    // Если подчинённое условие не одобрено – немедленно завершаем AND с false
    if (!SubCondition->IsApproved())
    {
        bFinalized = true;
        bCompleted = true;
        bApproved = false;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    // Иначе увеличиваем счётчик завершённых
    CompletedCount++;

    // Если все подчинённые условия завершены и все одобрены – результат true
    if (CompletedCount >= TotalValidConditions)
    {
        bFinalized = true;
        bCompleted = true;
        bApproved = true;
        OnComplete.ExecuteIfBound(this);
    }
}

FString UAndCheckCondition::GetDescription() const
{
    TArray<FString> Parts;
    for (const UCheckCondition* Sub : SubConditions)
    {
        if (Sub)
            Parts.Add(Sub->GetDescription());
        else
            Parts.Add(TEXT("Invalid"));
    }
    return FString::Printf(TEXT("AND( %s )"), *FString::Join(Parts, TEXT(" AND ")));
}