// InteriorCheckCondition.cpp
#include "InteriorCheckCondition.h"
#include "InteriorSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UInteriorCheckCondition::ExecuteCheck(const FGuid& TransactionId)
{
    // Сохраняем TransactionId для координатора
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    UWorld* World = GetWorld();
    if (!World)
    {
        bCompleted = true;
        bApproved = false;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    UGameInstance* GI = World->GetGameInstance();
    if (!GI)
    {
        bCompleted = true;
        bApproved = false;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    UInteriorSubsystem* InteriorSub = GI->GetSubsystem<UInteriorSubsystem>();
    if (!InteriorSub)
    {
        UE_LOG(LogTemp, Warning, TEXT("InteriorCheckCondition: InteriorSubsystem not found"));
        bCompleted = true;
        bApproved = false;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    // Получаем текущее количество
    int32 CurrentCount = InteriorSub->GetActorCountForCurrentFloor(CountType, FilterTypes);

    // Сравниваем с ожидаемым
    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (CurrentCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (CurrentCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (CurrentCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (CurrentCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (CurrentCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (CurrentCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;

    UE_LOG(LogTemp, Log, TEXT("InteriorCheckCondition: CountType=%d, FilterTypes=%d, Current=%d, Expected=%d, Approved=%s"),
        (int32)CountType, FilterTypes.Num(), CurrentCount, ExpectedValue, bApproved ? TEXT("true") : TEXT("false"));

    OnComplete.ExecuteIfBound(this);
}

FString UInteriorCheckCondition::GetDescription() const
{
    const TCHAR* TypeName = TEXT("Unknown");
    switch (CountType)
    {
    case EInteriorActorCountType::Registered: TypeName = TEXT("Registered"); break;
    case EInteriorActorCountType::Spawned:    TypeName = TEXT("Spawned"); break;
    case EInteriorActorCountType::Destroyed:  TypeName = TEXT("Destroyed"); break;
    }
    FString FilterStr = FilterTypes.Num() == 0 ? TEXT("All") : FString::Printf(TEXT("%d types"), FilterTypes.Num());
    return FString::Printf(TEXT("Interior Count [%s] filtered: %s, expected: %d"), TypeName, *FilterStr, ExpectedValue);
}