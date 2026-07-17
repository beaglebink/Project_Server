#include "InteriorClassTypeCheckCondition.h"
#include "FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InteriorSubsystem.h"

// ----------------------------------------------------------------------------
// GetFilteredObjectCount
// ----------------------------------------------------------------------------
int32 UInteriorClassTypeCheckCondition::GetFilteredObjectCount(UWorld* World) const
{
    if (!World)
        return 0;

    int32 Count = 0;

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        if (ActorClass && !Actor->IsA(ActorClass))
            continue;

        UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp)
            continue;

        if (Comp->ActorType != ActorType)
            continue;

        Count++;
    }

    return Count;
}

// ----------------------------------------------------------------------------
// Базовый ExecuteCheck (используется для UInteriorClassTypeCountCondition)
// ----------------------------------------------------------------------------
void UInteriorClassTypeCheckCondition::ExecuteCheck(const FGuid& TransactionId)
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
    LogVerbose(FString::Printf(TEXT("→ %s (count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), CurrentCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

// ----------------------------------------------------------------------------
// GetDescription для базового (если используется напрямую)
// ----------------------------------------------------------------------------
FString UInteriorClassTypeCheckCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType count [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ============================================================================
// Следующие классы наследуют ExecuteCheck и GetDescription,
// но для каждого из них нужно переопределить ExecuteCheck и GetDescription,
// чтобы использовать специфичные методы подсчёта.
// ============================================================================

// ----------------------------------------------------------------------------
// UInteriorClassTypeDestroyedCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeDestroyedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), DestroyedCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeDestroyedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType destroyed [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorClassTypeRemainingCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeRemainingCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (remaining=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), Remaining, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeRemainingCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType remaining [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorClassTypeSpawnedCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (spawn_count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), SpawnCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeSpawnedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType spawned [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorClassTypeDestroyedSpawnedCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeDestroyedSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), DestroyedSpawnedCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeDestroyedSpawnedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType destroyed spawned [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorClassTypeDestroyedOriginalCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeDestroyedOriginalCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), DestroyedOriginalCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeDestroyedOriginalCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType destroyed original [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorClassTypeRemainingSpawnedCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeRemainingSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), RemainingSpawnedCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeRemainingSpawnedCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType remaining spawned [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorClassTypeRemainingOriginalCondition
// ----------------------------------------------------------------------------
void UInteriorClassTypeRemainingOriginalCondition::ExecuteCheck(const FGuid& TransactionId)
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

    UGameInstance* GI = World->GetGameInstance();
    UInteriorSubsystem* InteriorSub = GI ? GI->GetSubsystem<UInteriorSubsystem>() : nullptr;
    if (!InteriorSub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no InteriorSub)"), false);
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
    LogVerbose(FString::Printf(TEXT("→ %s (count=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), RemainingOriginalCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorClassTypeRemainingOriginalCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("ClassType remaining original [%s, %s] %s %d"), *ClassName, *UEnum::GetValueAsString(ActorType), *UEnum::GetValueAsString(Operator), ExpectedValue);
}