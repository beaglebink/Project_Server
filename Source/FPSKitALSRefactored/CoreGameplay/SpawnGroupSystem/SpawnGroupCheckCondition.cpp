#include "SpawnGroupCheckCondition.h"
#include "SpawnGroupSubsystem.h"
#include "SpawnGroupSpawner.h"
#include "FloorAssignmentComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "CheckCoordinatorComponent.h"
#include "GameplayTagContainer.h"

// ============================================================================
// USpawnGroupConditionBase
// ============================================================================

USpawnGroupSubsystem* USpawnGroupConditionBase::GetSpawnGroupSubsystem() const
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;
    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;
    return GI->GetSubsystem<USpawnGroupSubsystem>();
}

void USpawnGroupConditionBase::ExecuteCheck(const FGuid& TransactionId)
{
    bCompleted = true;
    bApproved = false;
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupConditionBase::GetDescription() const
{
    return TEXT("SpawnGroup base condition");
}

#if WITH_EDITOR
void USpawnGroupConditionBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    DebugDescription = GetDescription();
}
#endif

// ============================================================================
// USpawnGroupCheckCondition
// ============================================================================

TArray<ASpawnGroupSpawner*> USpawnGroupCheckCondition::GetSpawners() const
{
    TArray<ASpawnGroupSpawner*> Result;
    for (const TSoftObjectPtr<ASpawnGroupSpawner>& SoftPtr : SpawnersReferences)
    {
        if (SoftPtr.IsValid())
        {
            ASpawnGroupSpawner* Spawner = SoftPtr.Get();
            if (Spawner && IsValid(Spawner))
                Result.Add(Spawner);
            else
            {
                Spawner = SoftPtr.LoadSynchronous();
                if (Spawner && IsValid(Spawner))
                    Result.Add(Spawner);
            }
        }
    }
    return Result;
}

TArray<FGuid> USpawnGroupCheckCondition::GetEffectiveGroupIds() const
{
    TArray<FGuid> Ids;
    TArray<ASpawnGroupSpawner*> Spawners = GetSpawners();
    for (ASpawnGroupSpawner* Spawner : Spawners)
    {
        if (UFloorAssignmentComponent* Comp = Spawner->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (Comp->ItemId.IsValid())
                Ids.Add(Comp->ItemId);
        }
    }
    return Ids;
}

bool USpawnGroupCheckCondition::EvaluateCompare(int32 ActualValue) const
{
    switch (Operator)
    {
    case ECheckCompareOp::Equal:          return ActualValue == ExpectedValue;
    case ECheckCompareOp::NotEqual:       return ActualValue != ExpectedValue;
    case ECheckCompareOp::Less:           return ActualValue < ExpectedValue;
    case ECheckCompareOp::LessOrEqual:    return ActualValue <= ExpectedValue;
    case ECheckCompareOp::Greater:        return ActualValue > ExpectedValue;
    case ECheckCompareOp::GreaterOrEqual: return ActualValue >= ExpectedValue;
    default: return false;
    }
}

FString USpawnGroupCheckCondition::GetDescription() const
{
    FString Target;
    if (SpawnersReferences.Num() > 0)
        Target = FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num());
    else
        Target = TEXT("Current Floor");
    return FString::Printf(TEXT("SpawnGroup count [%s] %s %d"), *Target, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ============================================================================
// 1. USpawnGroupStatusCondition
// ============================================================================

void USpawnGroupStatusCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    ASpawnGroupSpawner* SpawnerPtr = nullptr;
    if (Spawner.IsValid())
    {
        SpawnerPtr = Spawner.Get();
        if (!SpawnerPtr || !IsValid(SpawnerPtr))
            SpawnerPtr = Spawner.LoadSynchronous();
    }

    if (SpawnerPtr)
    {
        bApproved = (SpawnerPtr->GetCurrentStatus() == DesiredStatus);
        bCompleted = true;
        LogVerbose(FString::Printf(TEXT("→ %s (status=%d, desired=%d)"),
            bApproved ? TEXT("true") : TEXT("false"),
            static_cast<uint8>(SpawnerPtr->GetCurrentStatus()),
            static_cast<uint8>(DesiredStatus)), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    bApproved = false;
    bCompleted = true;
    LogVerbose(TEXT("→ false (no spawner specified)"), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupStatusCondition::GetDescription() const
{
    FString Target;
    if (Spawner.IsValid())
    {
        ASpawnGroupSpawner* S = Spawner.Get();
        if (S && IsValid(S))
            Target = FString::Printf(TEXT("Spawner '%s'"), *S->GetName());
        else
            Target = TEXT("Unknown Spawner");
    }
    else
        Target = TEXT("Not Set");

    FString StatusStr = StaticEnum<ESpawnGroupStatus>()->GetDisplayNameTextByValue(static_cast<int64>(DesiredStatus)).ToString();
    return FString::Printf(TEXT("SpawnGroup status [%s] == %s"), *Target, *StatusStr);
}

// ============================================================================
// 2. USpawnGroupTotalCountCondition
// ============================================================================

void USpawnGroupTotalCountCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetTotalSpawnedCount(Id);
    }
    else
    {
        Total = Sub->GetTotalSpawnedCountForCurrentFloor();
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (total=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupTotalCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup total count [%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 3. USpawnGroupAliveCountCondition
// ============================================================================

void USpawnGroupAliveCountCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetAliveCount(Id);
    }
    else
    {
        Total = Sub->GetAliveCountForCurrentFloor();
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup alive count [%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 4. USpawnGroupKilledCountCondition
// ============================================================================

void USpawnGroupKilledCountCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetKilledCount(Id);
    }
    else
    {
        Total = Sub->GetKilledCountForCurrentFloor();
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup killed count [%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 5. USpawnGroupCapturedCountCondition
// ============================================================================

void USpawnGroupCapturedCountCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetCapturedCount(Id);
    }
    else
    {
        Total = Sub->GetCapturedCountForCurrentFloor();
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (captured=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupCapturedCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup captured count [%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 6. USpawnGroupResolvedCountCondition (killed + captured)
// 6. USpawnGroupResolvedCountCondition (убитые + захваченные)
// ============================================================================

void USpawnGroupResolvedCountCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetResolvedCount(Id);
    }
    else
    {
        Total = Sub->GetResolvedCountForCurrentFloor();
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (resolved=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupResolvedCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup resolved count (killed+captured) [%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 7. USpawnGroupKilledByClassCondition (array of classes)
// 7. USpawnGroupKilledByClassCondition (массив классов)
// ============================================================================

void USpawnGroupKilledByClassCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetKilledCountByType(Id, ActorClasses);
    }
    else
    {
        Total = Sub->GetKilledCountByTypeForCurrentFloor(ActorClasses);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed_by_class=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledByClassCondition::GetDescription() const
{
    FString ClassesStr;
    for (const TSubclassOf<AActor>& Class : ActorClasses)
    {
        if (!ClassesStr.IsEmpty()) ClassesStr += TEXT(", ");
        ClassesStr += Class ? Class->GetName() : TEXT("None");
    }
    return FString::Printf(TEXT("SpawnGroup killed by classes [%s] classes=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *ClassesStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 8. USpawnGroupAliveByClassCondition
// ============================================================================

void USpawnGroupAliveByClassCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetAliveCountByType(Id, ActorClasses);
    }
    else
    {
        Total = Sub->GetAliveCountByTypeForCurrentFloor(ActorClasses);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive_by_class=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveByClassCondition::GetDescription() const
{
    FString ClassesStr;
    for (const TSubclassOf<AActor>& Class : ActorClasses)
    {
        if (!ClassesStr.IsEmpty()) ClassesStr += TEXT(", ");
        ClassesStr += Class ? Class->GetName() : TEXT("None");
    }
    return FString::Printf(TEXT("SpawnGroup alive by classes [%s] classes=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *ClassesStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 9. USpawnGroupCapturedByClassCondition
// ============================================================================

void USpawnGroupCapturedByClassCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetCapturedCountByType(Id, ActorClasses);
    }
    else
    {
        Total = Sub->GetCapturedCountByTypeForCurrentFloor(ActorClasses);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (captured_by_class=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupCapturedByClassCondition::GetDescription() const
{
    FString ClassesStr;
    for (const TSubclassOf<AActor>& Class : ActorClasses)
    {
        if (!ClassesStr.IsEmpty()) ClassesStr += TEXT(", ");
        ClassesStr += Class ? Class->GetName() : TEXT("None");
    }
    return FString::Printf(TEXT("SpawnGroup captured by classes [%s] classes=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *ClassesStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 10. USpawnGroupResolvedByClassCondition (killed + captured)
// 10. USpawnGroupResolvedByClassCondition (убитые + захваченные)
// ============================================================================

void USpawnGroupResolvedByClassCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetResolvedCountByType(Id, ActorClasses);
    }
    else
    {
        Total = Sub->GetResolvedCountByTypeForCurrentFloor(ActorClasses);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (resolved_by_class=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupResolvedByClassCondition::GetDescription() const
{
    FString ClassesStr;
    for (const TSubclassOf<AActor>& Class : ActorClasses)
    {
        if (!ClassesStr.IsEmpty()) ClassesStr += TEXT(", ");
        ClassesStr += Class ? Class->GetName() : TEXT("None");
    }
    return FString::Printf(TEXT("SpawnGroup resolved by classes (killed+captured) [%s] classes=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *ClassesStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 11. USpawnGroupKilledByTextTagCondition
// ============================================================================

void USpawnGroupKilledByTextTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetKilledCountByTextTag(Id, TextTags);
    }
    else
    {
        Total = Sub->GetKilledCountByTextTagForCurrentFloor(TextTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed_by_texttag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledByTextTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FName& Tag : TextTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup killed by text tags [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 12. USpawnGroupAliveByTextTagCondition
// ============================================================================

void USpawnGroupAliveByTextTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetAliveCountByTextTag(Id, TextTags);
    }
    else
    {
        Total = Sub->GetAliveCountByTextTagForCurrentFloor(TextTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive_by_texttag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveByTextTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FName& Tag : TextTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup alive by text tags [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 13. USpawnGroupCapturedByTextTagCondition
// ============================================================================

void USpawnGroupCapturedByTextTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetCapturedCountByTextTag(Id, TextTags);
    }
    else
    {
        Total = Sub->GetCapturedCountByTextTagForCurrentFloor(TextTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (captured_by_texttag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupCapturedByTextTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FName& Tag : TextTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup captured by text tags [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 14. USpawnGroupResolvedByTextTagCondition (killed + captured)
// 14. USpawnGroupResolvedByTextTagCondition (убитые + захваченные)
// ============================================================================

void USpawnGroupResolvedByTextTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetResolvedCountByTextTag(Id, TextTags);
    }
    else
    {
        Total = Sub->GetResolvedCountByTextTagForCurrentFloor(TextTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (resolved_by_texttag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupResolvedByTextTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FName& Tag : TextTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup resolved by text tags (killed+captured) [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 15. USpawnGroupKilledByGameplayTagCondition
// ============================================================================

void USpawnGroupKilledByGameplayTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetKilledCountByGameplayTag(Id, GameplayTags);
    }
    else
    {
        Total = Sub->GetKilledCountByGameplayTagForCurrentFloor(GameplayTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed_by_gptag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledByGameplayTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FGameplayTag& Tag : GameplayTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup killed by gameplay tags [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 16. USpawnGroupAliveByGameplayTagCondition
// ============================================================================

void USpawnGroupAliveByGameplayTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetAliveCountByGameplayTag(Id, GameplayTags);
    }
    else
    {
        Total = Sub->GetAliveCountByGameplayTagForCurrentFloor(GameplayTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive_by_gptag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveByGameplayTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FGameplayTag& Tag : GameplayTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup alive by gameplay tags [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 17. USpawnGroupCapturedByGameplayTagCondition
// ============================================================================

void USpawnGroupCapturedByGameplayTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetCapturedCountByGameplayTag(Id, GameplayTags);
    }
    else
    {
        Total = Sub->GetCapturedCountByGameplayTagForCurrentFloor(GameplayTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (captured_by_gptag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupCapturedByGameplayTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FGameplayTag& Tag : GameplayTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup captured by gameplay tags [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 18. USpawnGroupResolvedByGameplayTagCondition (killed + captured)
// 18. USpawnGroupResolvedByGameplayTagCondition (убитые + захваченные)
// ============================================================================

void USpawnGroupResolvedByGameplayTagCondition::ExecuteCheck(const FGuid& TransactionId)
{
    LogVerbose(TEXT(""), true);
    CurrentTransactionId = TransactionId;
    bCompleted = false;
    bApproved = false;

    USpawnGroupSubsystem* Sub = GetSpawnGroupSubsystem();
    if (!Sub)
    {
        bCompleted = true;
        bApproved = false;
        LogVerbose(TEXT("→ false (no subsystem)"), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    int32 Total = 0;
    TArray<FGuid> Ids = GetEffectiveGroupIds();
    if (Ids.Num() > 0)
    {
        for (const FGuid& Id : Ids)
            Total += Sub->GetResolvedCountByGameplayTag(Id, GameplayTags);
    }
    else
    {
        Total = Sub->GetResolvedCountByGameplayTagForCurrentFloor(GameplayTags);
    }

    bApproved = EvaluateCompare(Total);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (resolved_by_gptag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Total, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupResolvedByGameplayTagCondition::GetDescription() const
{
    FString TagsStr;
    for (const FGameplayTag& Tag : GameplayTags)
    {
        if (!TagsStr.IsEmpty()) TagsStr += TEXT(", ");
        TagsStr += Tag.ToString();
    }
    return FString::Printf(TEXT("SpawnGroup resolved by gameplay tags (killed+captured) [%s] tags=[%s] %s %d"),
        SpawnersReferences.Num() > 0 ? *FString::Printf(TEXT("%d spawners"), SpawnersReferences.Num()) : TEXT("CurrentFloor"),
        *TagsStr,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}