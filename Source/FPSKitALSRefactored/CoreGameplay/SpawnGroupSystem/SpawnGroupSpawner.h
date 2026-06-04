#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../SpawnGroupSystem/SpawnGroupAsset.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "../SpawnGroupSystem/SpawnVolume.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include <AlsCharacterExample.h>
#include "SpawnGroupSpawner.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnGroupGhostSpawned, AActor*, Ghost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpawnGroupGhostKilled, AActor*, Ghost, ESpawnGroupResolutionReason, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpawnGroupAllSpawned);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnGroupAllCleared, ESpawnGroupResolutionReason, Reason);

class UFloorAssignmentComponent;

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API ASpawnGroupSpawner : public AActor
{
    GENERATED_BODY()

public:
    ASpawnGroupSpawner();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    // ===== Конфигурация =====
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    TObjectPtr<USpawnGroupAsset> SpawnGroupAsset;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bSpawnOnBeginPlay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bDestroyOnClear = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
    bool bAllowRespawn = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup")
	float SpawnInterval = 0.1f; 

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    TArray<TObjectPtr<ASpawnVolume>> SpawnLocations;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    bool bAutoGatherSpawnLocations = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    FVector SpawnOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    FRotator DefaultSpawnRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SpawnGroup")
    TObjectPtr<UFloorAssignmentComponent> FloorAssignmentComp;

    UPROPERTY(VisibleAnywhere, SaveGame, Category = "SpawnGroup|State")
    ESpawnGroupStatus CurrentStatus = ESpawnGroupStatus::Inactive;

    UPROPERTY(BlueprintAssignable, Category = "SpawnGroup|Events")
    FOnSpawnGroupGhostSpawned OnGhostSpawned;

    UPROPERTY(BlueprintAssignable, Category = "SpawnGroup|Events")
    FOnSpawnGroupGhostKilled OnGhostKilled;

    UPROPERTY(BlueprintAssignable, Category = "SpawnGroup|Events")
    FOnSpawnGroupAllSpawned OnAllSpawned;

    UPROPERTY(BlueprintAssignable, Category = "SpawnGroup|Events")
    FOnSpawnGroupAllCleared OnAllCleared;

#if WITH_EDITOR
    virtual void Tick(float DeltaTime) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    void DrawEditorLines() const;
#endif

    // ===== Состояние =====
private:
    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    TArray<TObjectPtr<AAlsCharacter>> SpawnedGhosts;

    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    FSpawnGroupId RuntimeGroupId;

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
    bool bHasPublishedClear = false;

    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    int32 SpawnedCount = 0;

    UPROPERTY(VisibleAnywhere, SaveGame, Category = "SpawnGroup|State")
    int32 KilledCount = 0;

    FTimerHandle TimerHandle;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    TArray<AActor*> GetSpawnedGhosts() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    FSpawnGroupId GetRuntimeGroupId() const { return RuntimeGroupId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    ESpawnGroupStatus GetCurrentStatus() const { return CurrentStatus; }

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    void SpawnGroup();

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    void ClearGroup(ESpawnGroupResolutionReason Reason = ESpawnGroupResolutionReason::Eliminated);

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
    void ResetGroup();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup")
    int32 GetAliveGhostCount() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup")
    bool IsGroupCleared() const;

    void ResetKilledCount();

protected:
    ASpawnVolume* GetRandomSpawnLocation() const;
    FTransform GetTransformFromLocation(ASpawnVolume* Location) const;
    AActor* SpawnSingleGhost(TSubclassOf<AActor> ActorClass, const FTransform& Transform);
    void PublishGhostClearedEvent(ESpawnGroupResolutionReason Reason);
    void UpdateGroupStatus();
    void GatherSpawnLocationsFromChildren();

    UFUNCTION()
    void OnGhostDestroyed(AActor* DestroyedActor);
};