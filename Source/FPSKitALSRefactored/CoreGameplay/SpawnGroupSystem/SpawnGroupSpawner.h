#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../SpawnGroupSystem/SpawnGroupAsset.h"
#include "../SpawnGroupSystem/GhostClearedPayload.h"
#include "../SpawnGroupSystem/SpawnVolume.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include <AlsCharacterExample.h>
#include "FloorAssignmentComponent.h"
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
    virtual void OnConstruction(const FTransform& Transform) override;

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

    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "SpawnGroup|Placement")
    bool BlockNewSpawn = false;

    UPROPERTY(EditAnywhere, SaveGame, BlueprintReadWrite, Category = "SpawnGroup")
    bool IsStoreSpawnParameters = false;

    UPROPERTY(SaveGame)
    bool IsUseStoreSpawnParameters = false;

    UPROPERTY(SaveGame)
	TMap< TSubclassOf<AActor>, FTransform> StoredSpawnParameters;

    UPROPERTY(SaveGame)
    bool Restore = false;

    const TMap<FName, int32>& GetTypeKilled() const { return TypeKilled; }
    void RestoreFromState(const FSpawnGroupState& State);
    void RestoreFromSlots(const TArray<FSpawnSlotState>& Slots);
    TArray<FSpawnSlotState> CaptureCurrentSlots() const;
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
    FGuid RuntimeGroupId;

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
    bool bHasPublishedClear = false;

    UPROPERTY(VisibleAnywhere, Category = "SpawnGroup|State")
    int32 SpawnedCount = 0;

    UPROPERTY(VisibleAnywhere, SaveGame, Category = "SpawnGroup|State")
    int32 KilledCount = 0;

    FTimerHandle TimerHandle;

    UPROPERTY(SaveGame)
    TMap<FName, int32> TypeKilled;

    FSpawnGroupState StoredState;

public:
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    TArray<AActor*> GetSpawnedGhosts() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup|State")
    FGuid GetRuntimeGroupId()
    { 
        if (!RuntimeGroupId.IsValid() && FloorAssignmentComp)
        {
            //FSpawnGroupId Id;
            //Id.Id = 
            RuntimeGroupId = FloorAssignmentComp->ItemId;
                //Id;
            return RuntimeGroupId;//Id;
        }

        return RuntimeGroupId; 
    }

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
    void SpawnGroupInternal();

    int32 GetSpawnedCount() { return SpawnedCount; }
    int32 GetKilledCount() { return KilledCount; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "SpawnGroup")
    FGuid GetGroupId() const { return SpawnGroupAsset ? SpawnGroupAsset->GroupId : FGuid(); }

	void SetStates(const FSpawnGroupState& State);

    void StoreSpawnParameters();

    void SafeDestroyAllGhosts();

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