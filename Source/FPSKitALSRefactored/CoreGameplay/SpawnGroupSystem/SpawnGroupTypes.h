#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "GameplayTagContainer.h"
#include "SpawnGroupTypes.generated.h"

class USpawnGroupAsset;

// ============================================================================
// Базовые типы спавн-групп
// ============================================================================
UENUM(BlueprintType)
enum class EGhostState : uint8
{
    Alive       UMETA(DisplayName = "Alive"),
    Killed      UMETA(DisplayName = "Killed"),
    Captured    UMETA(DisplayName = "Captured")
};

UENUM(BlueprintType)
enum class ESpawnGroupStatus : uint8
{
    Inactive         UMETA(DisplayName = "Inactive"),
    Active           UMETA(DisplayName = "Active"),
    PartiallyCleared UMETA(DisplayName = "Partially Cleared"),
    Cleared          UMETA(DisplayName = "Cleared"),
    Suppressed       UMETA(DisplayName = "Suppressed")
};

UENUM(BlueprintType)
enum class ESpawnGroupResolutionReason : uint8
{
    None             UMETA(DisplayName = "None"),
    Eliminated       UMETA(DisplayName = "Eliminated"),
    Captured         UMETA(DisplayName = "Captured"),
    ScriptedRemoval  UMETA(DisplayName = "Scripted Removal"),
    Other            UMETA(DisplayName = "Other")
};

USTRUCT()
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
};

// ============================================================================
// Политика сохранения состояния спавнера
// ============================================================================
UENUM(BlueprintType)
enum class ESpawnerPreserveMode : uint8
{
    None                UMETA(DisplayName = "None (reset to fresh)"),
    Freeze              UMETA(DisplayName = "Freeze (keep partial state)"),
    FreezeUnlessCleared UMETA(DisplayName = "Freeze unless fully cleared")
};

// ============================================================================
// Payload для синхронного запроса политики через EventBus
// ============================================================================
UCLASS()
class FPSKITALSREFACTORED_API USpawnerPreserveModePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FGuid InteriorSetId;

    UPROPERTY()
    FGuid FloorId;

    UPROPERTY()
    ESpawnerPreserveMode PreserveMode = ESpawnerPreserveMode::None;
};

// ============================================================================
// Структуры состояния спавнера
// ============================================================================
USTRUCT()
struct FSpawnSlotState
{
    GENERATED_BODY()

    UPROPERTY()
    FGuid ItemId;

    UPROPERTY()
    TSubclassOf<AActor> ActorClass = nullptr;

    UPROPERTY()
    FTransform SpawnTransform = FTransform::Identity;

    UPROPERTY()
    EGhostState State = EGhostState::Alive;

    UPROPERTY()
    FGameplayTagContainer GameplayTags;

    UPROPERTY()
    TArray<FName> TextTags;
};

USTRUCT()
struct FSpawnGroupState
{
    GENERATED_BODY()

    UPROPERTY() FGuid GroupId;
    UPROPERTY() ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;
    UPROPERTY() ESpawnGroupResolutionReason ResolutionReason = ESpawnGroupResolutionReason::ScriptedRemoval;
    UPROPERTY() bool bStoreSpawnParameters = false;
    UPROPERTY() bool bBlockNewSpawn = false;
    UPROPERTY() int32 KilledCount = 0;
    UPROPERTY() TSoftObjectPtr<USpawnGroupAsset> SpawnGroupAssetOverride = nullptr;
    UPROPERTY() TArray<FSpawnSlotState> Slots;
    UPROPERTY() TMap<FName, int32> TypeKilled;
    UPROPERTY() bool bManualActivationOnly = false;
};
