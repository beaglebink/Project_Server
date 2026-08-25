// SpawnGroupTypes.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"  
#include "GameplayTagContainer.h"
#include "SpawnGroupTypes.generated.h"

USTRUCT(BlueprintType)
struct FSpawnedEnemyState
{
    GENERATED_BODY()

    UPROPERTY()
    FGuid ItemId;

    UPROPERTY()
    TSubclassOf<AActor> ActorClass;

    UPROPERTY()
    FTransform SpawnTransform;

    UPROPERTY()
    FString SerializedState;

    FSpawnedEnemyState()
        : ItemId(FGuid())
        , ActorClass(nullptr)
        , SpawnTransform(FTransform::Identity)
        , SerializedState(TEXT(""))
    {
    }
};
USTRUCT(BlueprintType)
struct FSpawnGroupId
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid Id;
    FSpawnGroupId() {}
    explicit FSpawnGroupId(const FGuid& InId) : Id(InId) {}
    bool IsValid() const { return Id.IsValid(); }
    bool operator==(const FSpawnGroupId& Other) const { return Id == Other.Id; }
    FString ToString() const { return Id.ToString(); }
    static FSpawnGroupId New() { return FSpawnGroupId(FGuid::NewGuid()); }
};

FORCEINLINE uint32 GetTypeHash(const FSpawnGroupId& Key)
{
    return GetTypeHash(Key.Id);
}

UENUM(BlueprintType)
enum class ESpawnGroupStatus : uint8
{
    Inactive,
    Active,
    PartiallyCleared,
    Cleared,
    Suppressed
};

UENUM(BlueprintType)
enum class ESpawnGroupResolutionReason : uint8
{
    None,
    Eliminated,
    Captured,
    ScriptedRemoval,
    Other
};

USTRUCT(BlueprintType)
struct FSpawnGroupRecord
{
    GENERATED_BODY()
    UPROPERTY()
    TWeakObjectPtr<AActor> SpawnedActor;
    UPROPERTY()
    TSubclassOf<AActor> ActorClass;
    UPROPERTY()
    FTransform SpawnTransform;
    UPROPERTY()
    bool bIsResolved = false;
    UPROPERTY()
    ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::None;
};

UENUM(BlueprintType)
enum class EGhostState : uint8
{
    Alive     UMETA(DisplayName = "Alive"),
    Captured  UMETA(DisplayName = "Captured"),
    Killed    UMETA(DisplayName = "Killed")
};

USTRUCT(BlueprintType)
struct FSpawnSlotState
{
    GENERATED_BODY()
    UPROPERTY()
    FGuid ItemId;
    UPROPERTY()
    TSubclassOf<AActor> ActorClass;
    UPROPERTY()
    FTransform SpawnTransform = FTransform::Identity;
    UPROPERTY()
    EGhostState State = EGhostState::Alive;
    UPROPERTY()
    FGameplayTagContainer GameplayTags;
    UPROPERTY()
    TArray<FName> TextTags;
};

USTRUCT(BlueprintType)
struct FSpawnGroupState
{
    GENERATED_BODY()
    UPROPERTY()
    FGuid GroupId;
    UPROPERTY()
    ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;
    UPROPERTY()
    ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::None;
    UPROPERTY()
    FName LastMissionContext;
    UPROPERTY()
    int32 VisitIndex = 0;
    UPROPERTY()
    int32 KilledCount = 0;
    UPROPERTY()
    TMap<FName, int32> TypeKilled;
    UPROPERTY()
    TArray<FSpawnSlotState> Slots;
    UPROPERTY()
    bool bStoreSpawnParameters = false;

    UPROPERTY()
    TArray<FSpawnedEnemyState> EnemyStates; // состояние каждого врага
};

