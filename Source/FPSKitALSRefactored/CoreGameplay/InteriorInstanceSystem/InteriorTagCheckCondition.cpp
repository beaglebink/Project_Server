#include "InteriorTagCheckCondition.h"
#include "FloorAssignmentComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "InteriorSubsystem.h"
#include "GameplayTagContainer.h"

// ----------------------------------------------------------------------------
// GetTaggedObjectCount
// ----------------------------------------------------------------------------
int32 UInteriorTagCheckCondition::GetTaggedObjectCount(UWorld* World) const
{
    if (!World)
        return 0;

    int32 Count = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* Actor = *It;
        if (!IsValid(Actor))
            continue;

        UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp)
            continue;

        bool bMatches = false;
        if (TagType == ETagType::TextTag)
        {
            if (TextTag != NAME_None && Actor->Tags.Contains(TextTag))
                bMatches = true;
        }
        else // GameplayTag
        {
            if (GameplayTag.IsValid() && Comp->GameplayTagContainer.HasTag(GameplayTag))
                bMatches = true;
        }

        if (bMatches)
            Count++;
    }
    return Count;
}

// ----------------------------------------------------------------------------
// Базовый ExecuteCheck (используется для UInteriorTagCountCondition)
// ----------------------------------------------------------------------------
void UInteriorTagCheckCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 CurrentCount = GetTaggedObjectCount(World);
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
// GetDescription для базового
// ----------------------------------------------------------------------------
FString UInteriorTagCheckCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ============================================================================
// Наследники
// ============================================================================

// ----------------------------------------------------------------------------
// UInteriorTagDestroyedCondition
// ----------------------------------------------------------------------------
void UInteriorTagDestroyedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 DestroyedCount = 0;
    if (TagType == ETagType::TextTag)
        DestroyedCount = InteriorSub->GetDestroyedTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    else
        DestroyedCount = InteriorSub->GetDestroyedTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);

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

FString UInteriorTagDestroyedCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag destroyed count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorTagRemainingCondition
// ----------------------------------------------------------------------------
void UInteriorTagRemainingCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 Total = GetTaggedObjectCount(World);
    int32 DestroyedCount = 0;
    if (TagType == ETagType::TextTag)
        DestroyedCount = InteriorSub->GetDestroyedTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    else
        DestroyedCount = InteriorSub->GetDestroyedTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);

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
    LogVerbose(FString::Printf(TEXT("→ %s (remaining=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), Remaining, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorTagRemainingCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag remaining count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorTagSpawnedCondition
// ----------------------------------------------------------------------------
void UInteriorTagSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 SpawnedRemainingCount = 0;
    int32 DestroyedSpawnedCount = 0;
    if (TagType == ETagType::TextTag)
    {
        SpawnedRemainingCount = InteriorSub->GetSpawnedTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
        DestroyedSpawnedCount = InteriorSub->GetDestroyedSpawnedTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    }
    else
    {
        SpawnedRemainingCount = InteriorSub->GetSpawnedTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);
        DestroyedSpawnedCount = InteriorSub->GetDestroyedSpawnedTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);
    }
    int32 SpawnedCount = SpawnedRemainingCount + DestroyedSpawnedCount;

    switch (Operator)
    {
    case ECheckCompareOp::Equal:          bApproved = (SpawnedCount == ExpectedValue); break;
    case ECheckCompareOp::NotEqual:       bApproved = (SpawnedCount != ExpectedValue); break;
    case ECheckCompareOp::Less:           bApproved = (SpawnedCount < ExpectedValue); break;
    case ECheckCompareOp::LessOrEqual:    bApproved = (SpawnedCount <= ExpectedValue); break;
    case ECheckCompareOp::Greater:        bApproved = (SpawnedCount > ExpectedValue); break;
    case ECheckCompareOp::GreaterOrEqual: bApproved = (SpawnedCount >= ExpectedValue); break;
    default: bApproved = false; break;
    }

    bCompleted = true;
    LogVerbose(FString::Printf(TEXT("→ %s (spawned_total=%d, expected=%d)"), bApproved ? TEXT("true") : TEXT("false"), SpawnedCount, ExpectedValue), false);
    OnComplete.ExecuteIfBound(this);
}

FString UInteriorTagSpawnedCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag spawned (alive) count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorTagDestroyedSpawnedCondition
// ----------------------------------------------------------------------------
void UInteriorTagDestroyedSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 DestroyedSpawnedCount = 0;
    if (TagType == ETagType::TextTag)
        DestroyedSpawnedCount = InteriorSub->GetDestroyedSpawnedTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    else
        DestroyedSpawnedCount = InteriorSub->GetDestroyedSpawnedTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);

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

FString UInteriorTagDestroyedSpawnedCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag destroyed spawned count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorTagDestroyedOriginalCondition
// ----------------------------------------------------------------------------
void UInteriorTagDestroyedOriginalCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 DestroyedOriginalCount = 0;
    if (TagType == ETagType::TextTag)
        DestroyedOriginalCount = InteriorSub->GetDestroyedOriginalTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    else
        DestroyedOriginalCount = InteriorSub->GetDestroyedOriginalTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);

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

FString UInteriorTagDestroyedOriginalCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag destroyed original count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorTagRemainingSpawnedCondition
// ----------------------------------------------------------------------------
void UInteriorTagRemainingSpawnedCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 RemainingSpawnedCount = 0;
    if (TagType == ETagType::TextTag)
        RemainingSpawnedCount = InteriorSub->GetRemainingSpawnedTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    else
        RemainingSpawnedCount = InteriorSub->GetRemainingSpawnedTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);

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

FString UInteriorTagRemainingSpawnedCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag remaining spawned count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}

// ----------------------------------------------------------------------------
// UInteriorTagRemainingOriginalCondition
// ----------------------------------------------------------------------------
void UInteriorTagRemainingOriginalCondition::ExecuteCheck(const FGuid& TransactionId)
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

    int32 RemainingOriginalCount = 0;
    if (TagType == ETagType::TextTag)
        RemainingOriginalCount = InteriorSub->GetRemainingOriginalTagCountForCurrentFloor(TagType, TextTag, FGameplayTag());
    else
        RemainingOriginalCount = InteriorSub->GetRemainingOriginalTagCountForCurrentFloor(TagType, NAME_None, GameplayTag);

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

FString UInteriorTagRemainingOriginalCondition::GetDescription() const
{
    FString TagStr = (TagType == ETagType::TextTag) ? TextTag.ToString() : GameplayTag.ToString();
    return FString::Printf(TEXT("Tag remaining original count [%s] %s %d"), *TagStr, *UEnum::GetValueAsString(Operator), ExpectedValue);
}