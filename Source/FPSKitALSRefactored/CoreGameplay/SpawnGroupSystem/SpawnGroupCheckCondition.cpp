// SpawnGroupCheckCondition.cpp
#include "SpawnGroupCheckCondition.h"
#include "SpawnGroupSubsystem.h"
#include "SpawnGroupSpawner.h"
#include "FloorAssignmentComponent.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "CheckCoordinatorComponent.h"
#include "GameplayTagContainer.h"

// ============================================================================
// USpawnGroupConditionBase – общая логика для всех условий
// ============================================================================

ASpawnGroupSpawner* USpawnGroupConditionBase::GetSpawner() const
{
    if (SpawnerReference.IsValid())
    {
        ASpawnGroupSpawner* Spawner = SpawnerReference.Get();
        if (Spawner && IsValid(Spawner))
            return Spawner;
        Spawner = SpawnerReference.LoadSynchronous();
        if (Spawner && IsValid(Spawner))
            return Spawner;
    }
    return nullptr;
}

FGuid USpawnGroupConditionBase::GetEffectiveGroupId() const
{
    if (ASpawnGroupSpawner* Spawner = GetSpawner())
    {
        if (UFloorAssignmentComponent* Comp = Spawner->FindComponentByClass<UFloorAssignmentComponent>())
        {
            if (Comp->ItemId.IsValid())
                return Comp->ItemId;
        }
        if (Spawner->SpawnGroupAsset && Spawner->SpawnGroupAsset->GroupId.IsValid())
            return Spawner->SpawnGroupAsset->GroupId;
    }
    return FGuid(); // пустой – значит проверка по этажу
}

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
    // Базовый метод – заглушка, переопределяется в наследниках
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
// USpawnGroupCheckCondition – количественные проверки (с оператором)
// ============================================================================

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
    return FString::Printf(TEXT("SpawnGroup count [%s] %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 1. USpawnGroupStatusCondition (без оператора)
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

    if (ASpawnGroupSpawner* Spawner = GetSpawner())
    {
        bApproved = (Spawner->GetCurrentStatus() == DesiredStatus);
        bCompleted = true;
        LogVerbose(FString::Printf(TEXT("→ %s (status=%d, desired=%d)"),
            bApproved ? TEXT("true") : TEXT("false"),
            static_cast<uint8>(Spawner->GetCurrentStatus()),
            static_cast<uint8>(DesiredStatus)), false);
        OnComplete.ExecuteIfBound(this);
        return;
    }

    // Если спавнер не задан – проверка по этажу (заглушка)
    bApproved = false;
    bCompleted = true;
    LogVerbose(TEXT("→ false (floor aggregation not implemented)"), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupStatusCondition::GetDescription() const
{
    FString Target;
    if (GetSpawner())
        Target = FString::Printf(TEXT("Spawner '%s'"), *GetSpawner()->GetName());
    else
        Target = TEXT("Current Floor (aggregated)");

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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetTotalSpawnedCount(EffectiveId);
    else
        Actual = Sub->GetTotalSpawnedCountForCurrentFloor();

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (total=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupTotalCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup total count [%s] %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetAliveCount(EffectiveId);
    else
        Actual = Sub->GetAliveCountForCurrentFloor();

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup alive count [%s] %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetKilledCount(EffectiveId);
    else
        Actual = Sub->GetKilledCountForCurrentFloor();

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledCountCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup killed count [%s] %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 5. USpawnGroupKilledByTypeCondition
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetKilledCountByType(EffectiveId, ActorClass);
    else
        Actual = Sub->GetKilledCountByTypeForCurrentFloor(ActorClass);

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed_by_type=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledByClassCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("SpawnGroup killed by type [%s] class=%s %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *ClassName,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 6. USpawnGroupAliveByTypeCondition
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetAliveCountByType(EffectiveId, ActorClass);
    else
        Actual = Sub->GetAliveCountByTypeForCurrentFloor(ActorClass);

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive_by_type=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveByClassCondition::GetDescription() const
{
    FString ClassName = ActorClass ? ActorClass->GetName() : TEXT("Any");
    return FString::Printf(TEXT("SpawnGroup alive by type [%s] class=%s %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *ClassName,
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 7. USpawnGroupKilledByTextTagCondition
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetKilledCountByTextTag(EffectiveId, TextTag);
    else
        Actual = Sub->GetKilledCountByTextTagForCurrentFloor(TextTag);

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed_by_texttag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledByTextTagCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup killed by text tag [%s] tag=%s %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *TextTag.ToString(),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 8. USpawnGroupAliveByTextTagCondition
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetAliveCountByTextTag(EffectiveId, TextTag);
    else
        Actual = Sub->GetAliveCountByTextTagForCurrentFloor(TextTag);

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive_by_texttag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveByTextTagCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup alive by text tag [%s] tag=%s %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *TextTag.ToString(),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 9. USpawnGroupKilledByGameplayTagCondition
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetKilledCountByGameplayTag(EffectiveId, GameplayTag);
    else
        Actual = Sub->GetKilledCountByGameplayTagForCurrentFloor(GameplayTag);

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (killed_by_gptag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupKilledByGameplayTagCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup killed by gameplay tag [%s] tag=%s %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *GameplayTag.ToString(),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}

// ============================================================================
// 10. USpawnGroupAliveByGameplayTagCondition
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

    int32 Actual = 0;
    FGuid EffectiveId = GetEffectiveGroupId();
    if (EffectiveId.IsValid())
        Actual = Sub->GetAliveCountByGameplayTag(EffectiveId, GameplayTag);
    else
        Actual = Sub->GetAliveCountByGameplayTagForCurrentFloor(GameplayTag);

    bApproved = EvaluateCompare(Actual);
    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (alive_by_gptag=%d, expected=%d)"),
        bApproved ? TEXT("true") : TEXT("false"), Actual, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString USpawnGroupAliveByGameplayTagCondition::GetDescription() const
{
    return FString::Printf(TEXT("SpawnGroup alive by gameplay tag [%s] tag=%s %s %d"),
        GetSpawner() ? *GetSpawner()->GetName() : TEXT("CurrentFloor"),
        *GameplayTag.ToString(),
        *UEnum::GetValueAsString(Operator),
        ExpectedValue);
}