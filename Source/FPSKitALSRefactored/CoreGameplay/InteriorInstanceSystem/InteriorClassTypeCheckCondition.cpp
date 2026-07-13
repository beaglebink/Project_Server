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

    // Iterate over all actors in the world
    // Перебор всех акторов в мире
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        // Filter by class (if set)
        // Фильтр по классу (если задан)
        if (ActorClass && !Actor->IsA(ActorClass))
            continue;

        // Check for FloorAssignment component
        // Проверяем наличие компонента FloorAssignment
        UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp)
            continue;

        // Filter by EFloorActorType
        // LightItem – default value meaning "all types"
        // Фильтр по типу EFloorActorType
        // LightItem – дефолтное значение, означающее "все типы"
        if (Comp->ActorType != ActorType)
            continue;

        // Object passed all filters
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

    int32 DestroyedCount = InteriorSub->GetDestroyedActorCountForCurrentFloor(ActorClass, ActorType);
    int32 Remaining = GetFilteredObjectCount(World);

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

void UInteriorClassTypeSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 SpawnedRemainingCount = InteriorSub->GetSpawnedActorCountForCurrentFloor(ActorClass, ActorType);
    int32 DestroyedSpawnedCount = InteriorSub->GetDestroyedSpawnedActorCountForCurrentFloor(ActorClass, ActorType);
    int32 SpawnCount = SpawnedRemainingCount + DestroyedSpawnedCount;
    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (SpawnCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (SpawnCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (SpawnCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (SpawnCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (SpawnCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (SpawnCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }
    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeSpawnedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType spawned [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// Addition
// дополнение

void UInteriorClassTypeDestroyedSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 DestroyedSpawnedCount = InteriorSub->GetDestroyedSpawnedActorCountForCurrentFloor(ActorClass, ActorType);

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (DestroyedSpawnedCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (DestroyedSpawnedCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (DestroyedSpawnedCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (DestroyedSpawnedCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (DestroyedSpawnedCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (DestroyedSpawnedCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeDestroyedSpawnedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType destroyed spawned [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

void UInteriorClassTypeDestroyedOriginalCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 DestroyedOriginalCount = InteriorSub->GetDestroyedOriginalActorCountForCurrentFloor(ActorClass, ActorType);

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (DestroyedOriginalCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (DestroyedOriginalCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (DestroyedOriginalCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (DestroyedOriginalCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (DestroyedOriginalCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (DestroyedOriginalCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeDestroyedOriginalCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType destroyed original [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

void UInteriorClassTypeRemainingSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 RemainingSpawnedCount = InteriorSub->GetRemainingSpawnedActorCountForCurrentFloor(ActorClass, ActorType);

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (RemainingSpawnedCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (RemainingSpawnedCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (RemainingSpawnedCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (RemainingSpawnedCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (RemainingSpawnedCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (RemainingSpawnedCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeRemainingSpawnedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType remaining spawned [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

void UInteriorClassTypeRemainingOriginalCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 RemainingOriginalCount = InteriorSub->GetRemainingOriginalActorCountForCurrentFloor(ActorClass, ActorType);

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (RemainingOriginalCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (RemainingOriginalCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (RemainingOriginalCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (RemainingOriginalCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (RemainingOriginalCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (RemainingOriginalCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeRemainingOriginalCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType remaining original [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}