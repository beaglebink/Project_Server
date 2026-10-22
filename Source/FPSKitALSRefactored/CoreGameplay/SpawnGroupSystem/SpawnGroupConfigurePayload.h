#pragma once

#include "CoreMinimal.h"
#include "SpawnGroupLocationPayloadBase.h"
#include "SpawnGroupTypes.h"
#include "GameplayTagContainer.h"
#include "SpawnGroupConfigurePayload.generated.h"

class USpawnGroupAsset;
class ASpawnVolume;

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API USpawnGroupConfigurePayload : public USpawnGroupLocationPayloadBase
{
    GENERATED_BODY()

public:
    // Asset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Asset")
    TObjectPtr<USpawnGroupAsset> NewSpawnGroupAsset = nullptr;

    // Status
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (InlineEditConditionToggle))
    bool bApplyStatus = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (EditCondition = "bApplyStatus"))
    ESpawnGroupStatus Status = ESpawnGroupStatus::Inactive;

    // BlockNewSpawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (InlineEditConditionToggle))
    bool bApplyBlockNewSpawn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (EditCondition = "bApplyBlockNewSpawn"))
    bool bBlockNewSpawn = false;

    // Manual activation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (InlineEditConditionToggle))
    bool bApplyManualActivationOnly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (EditCondition = "bApplyManualActivationOnly"))
    bool bManualActivationOnly = false;

    // Store spawn parameters
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (InlineEditConditionToggle))
    bool bApplyStoreSpawnParameters = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (EditCondition = "bApplyStoreSpawnParameters"))
    bool IsStoreSpawnParameters = false;

    // Saving health
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (InlineEditConditionToggle))
    bool bApplySavingHealth = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Runtime",
        meta = (EditCondition = "bApplySavingHealth"))
    bool IsSavingHealth = true;

    // Spawn on BeginPlay
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Behaviour",
    //    meta = (InlineEditConditionToggle))
    bool bApplySpawnOnBeginPlay = false;

    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Behaviour",
    //    meta = (EditCondition = "bApplySpawnOnBeginPlay"))
    bool bSpawnOnBeginPlay = false;

    // Destroy on clear
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Behaviour",
        meta = (InlineEditConditionToggle))
    bool bApplyDestroyOnClear = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Behaviour",
        meta = (EditCondition = "bApplyDestroyOnClear"))
    bool bDestroyOnClear = false;

    // Allow respawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Behaviour",
        meta = (InlineEditConditionToggle))
    bool bApplyAllowRespawn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Behaviour",
        meta = (EditCondition = "bApplyAllowRespawn"))
    bool bAllowRespawn = false;

    // Spawn interval
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (InlineEditConditionToggle))
    bool bApplySpawnInterval = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (EditCondition = "bApplySpawnInterval", ClampMin = "0.0", UIMin = "0.0"))
    float SpawnInterval = 0.05f;

    // Spawn offset
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (InlineEditConditionToggle))
    bool bApplySpawnOffset = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (EditCondition = "bApplySpawnOffset"))
    FVector SpawnOffset = FVector::ZeroVector;

    // Default spawn rotation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (InlineEditConditionToggle))
    bool bApplyDefaultSpawnRotation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (EditCondition = "bApplyDefaultSpawnRotation"))
    FRotator DefaultSpawnRotation = FRotator::ZeroRotator;

    // Spawn locations (soft)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (InlineEditConditionToggle))
    bool bApplySpawnLocations = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Spawn",
        meta = (EditCondition = "bApplySpawnLocations"))
    TArray<TSoftObjectPtr<ASpawnVolume>> SpawnLocations;

    // Enemy tags
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Tags",
        meta = (InlineEditConditionToggle))
    bool bApplyEnemyTags = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Tags",
        meta = (EditCondition = "bApplyEnemyTags"))
    TArray<FName> EnemyTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Tags",
        meta = (InlineEditConditionToggle))
    bool bApplyEnemyGameplayTags = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Tags",
        meta = (EditCondition = "bApplyEnemyGameplayTags"))
    FGameplayTagContainer EnemyGameplayTags;

    // Debug
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Debug",
        meta = (InlineEditConditionToggle))
    bool bApplyDebugSpawnPoints = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Configure|Debug",
        meta = (EditCondition = "bApplyDebugSpawnPoints"))
    bool DebugSpawnPoints = false;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Configure")
    USpawnGroupConfigurePayload* Setup(const FGuid& InGroupId)
    {
        GroupId = InGroupId;
        return this;
    }
};