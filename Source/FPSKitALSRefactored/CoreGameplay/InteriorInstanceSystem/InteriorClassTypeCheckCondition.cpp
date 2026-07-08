// InteriorClassTypeCheckCondition.cpp
#include "InteriorClassTypeCheckCondition.h"
#include "FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InteriorSubsystem.h"

int32 UInteriorClassTypeCheckCondition::GetFilteredObjectCount(UWorld* World) const
{
    if (!World)
        return 0;

    int32 Count = 0;

    // Перебор всех акторов в мире
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        // Фильтр по классу (если задан)
        if (ActorClass && !Actor->IsA(ActorClass))
            continue;

        // Проверяем наличие компонента FloorAssignment
        UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp)
            continue;

        // Фильтр по типу EFloorActorType
        // LightItem – дефолтное значение, означающее "все типы"
        if (Comp->ActorType != ActorType)
            continue;

        // Объект прошёл все фильтры
        Count++;
    }

    return Count;
}

void UInteriorClassTypeCheckCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 CurrentCount = GetFilteredObjectCount(World);
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
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeCheckCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType count [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

void UInteriorClassTypeDestroyedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 DestroyedCount = InteriorSub->GetDestroyedActorCountForCurrentFloor(ActorClass, ActorType);

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (DestroyedCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (DestroyedCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (DestroyedCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (DestroyedCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (DestroyedCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (DestroyedCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeDestroyedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType destroyed [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----- UInteriorClassTypeRemainingCondition -----
void UInteriorClassTypeRemainingCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        OnComplete.ExecuteIfBound(this);
        return;
    }

    // Общее количество существующих
    int32 Total = GetFilteredObjectCount(World);
    // Уничтоженные
    int32 DestroyedCount = InteriorSub->GetDestroyedActorCountForCurrentFloor(ActorClass, ActorType);
    int32 Remaining = Total - DestroyedCount;

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (Remaining == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (Remaining != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (Remaining < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (Remaining <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (Remaining > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (Remaining >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeRemainingCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType remaining [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}