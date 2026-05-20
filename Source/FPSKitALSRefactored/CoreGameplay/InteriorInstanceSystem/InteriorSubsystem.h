#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h" 
#include "FloorPopulationTypes.h"
#include "FloorPlacementPayload.h"
#include "../LocationSystem/LocationSpatialTypes.h"
#include <InteriorTransitionPayload.h>
#include "../MissionSystem/MissionEnvelopeTypes.h"
#include "ISaveableSubsystem.h"
#include "MissionAsset.h"
#include "InteriorSubsystem.generated.h"

class UMissionController;
class UInteractiveItemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTransitionCompleted, bool, bSuccess, FLocationAnchorLink, DestinationLink, bool, IsTravel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorExiting, TSoftObjectPtr<UFloorAsset>, SourceFloor);

struct FInteractItemRecord
{
    FGuid ItemId;
    EInteractiveSubsystem SubsystemType;
    float InteractionRange;
    FText DefaultTooltip;
    TWeakObjectPtr<AActor> OwnerActor;
};

struct FFloorSavedPropertyEntry
{
    FName  PropertyName;
    FString ValueText;
};

struct FFloorSavedComponentState
{
    FName ComponentName;
    FName ComponentClassName;
    bool  bWasActive = true;
    bool  bWasAttached = false;
    FName AttachParentName;
    FName AttachSocketName;
    FTransform RelativeTransform;
    FTransform WorldTransform;
    bool bHasRelativeTransform = false;
    bool bHasWorldTransform = true;
    bool bWasSimulatingPhysics = false;
    FVector SavedLinearVelocity = FVector::ZeroVector;
    FVector SavedAngularVelocityDeg = FVector::ZeroVector;
    TArray<FFloorSavedPropertyEntry> Properties;
};

struct FFloorSavedActorState
{
    FGuid      ItemId;
    FTransform ActorTransform;
    FTransform RelativeTransform;
    bool       bHasRelativeTransform = false;
    TArray<FFloorSavedPropertyEntry>  ActorProperties;
    TArray<FFloorSavedComponentState> ComponentStates;
};

USTRUCT()
struct FActiveMissionInterior
{
    GENERATED_BODY()

    UPROPERTY()
    FName MissionId;

    UPROPERTY()
    int32 MissionStep;

    UPROPERTY()
    TObjectPtr<UMissionController> Controller;
};

USTRUCT()
struct FDelayedClear
{
    GENERATED_BODY()

    UPROPERTY()
    FName MissionId;

    UPROPERTY()
    FMissionEnvelope Envelope;

    UPROPERTY()
    EJobSpacePolicy Policy;

    UPROPERTY()
    TArray<FEnvelopeChannelEntry> EndChannels;

    UPROPERTY()
    bool IsValid = false;
};

UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods, public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void SubscribeToSpawnActor();
    void UnsubscribeFromSpawnActor();
    void OnActorSpawned(AActor* SpawnedActor);
    void TryRegisterActor(AActor* SpawnedActor, int32 AttemptsLeft);

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void SubscribeInteractionRegistration();
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void UnsubscribeInteractionRegistration();

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void SubscribeInteractCommand();
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void UnsubscribeInteractCommand();

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void SubscribeSetEnabled();
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void UnsubscribeSetEnabled();

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void SubscribeSetRange();
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void UnsubscribeSetRange();

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void SubscribeSetTooltip();
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
    void UnsubscribeSetTooltip();

    void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
    void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

    UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
    FOnInteriorInteractItemRegistrationEvent OnInteractItemRegistered;
    UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
    FOnInteriorInteractItemRegistrationEvent OnInteractItemUnregistered;
    UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
    FOnTransitionCompleted OnTransitionCompleted;
    UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
    FOnFloorExiting OnFloorExiting;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Population")
    TArray<FFloorPopulationRecord> GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
    void SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason, bool IsSaveAll = false);
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
    int32 RestoreFloorActorsState(const FGuid& InteriorSetId, int32 CurrentMissionStep, const FGuid& FloorId);
    int32 RestoreFromSnapshotArray(UWorld* W, const TArray<FFloorSavedActorState>& Snapshots, const FGuid& InteriorSetId, const FGuid& FloorId, const FMissionEnvelope Envelope);

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
    void SetPendingSpawnTransform(const FVector& Location, const FRotator& Rotation);
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
    void GetPendingSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const;
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
    void ClearPendingSpawnTransform();
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem|Transition")
    bool HasPendingSpawnTransform() const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
    void SetPendingAnchorID(const FGuid& AnchorID);
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem|Transition")
    FGuid GetPendingAnchorID() const;
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
    void ClearPendingAnchorID();
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem|Transition")
    bool HasPendingAnchorID() const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
    bool TeleportToAnchor(const FGuid& AnchorID);

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Placement")
    void SubscribePlacementRegistration();
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Placement")
    void UnsubscribePlacementRegistration();

    void HandlePlacementRegistration(const FOutcomeEventBase& Outcome);

    bool IsCurrentWorldMission(FMissionEnvelope& Envelope);

    bool IsCurrentWorldMissionConst(const FMissionEnvelope& Envelope);

    void SaveMissionFloorState(FName MissionId, const FInteriorFloorKey& FloorKey);
    void RestoreMissionFloorState(FName MissionId, int32 CurrentMissionStep, const FInteriorFloorKey& FloorKey);
    void SpawnFromType(TArray<FFloorPopulationRecord>& Array, const FMissionEnvelope& Envelope);
    void RestoreSpawnedActorsForCurrentFloor(const FMissionEnvelope& Envelope);
    void ReleaseMissionSnapshot(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, bool bIsCompletion = false);
    const TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>* GetMissionSnapshots(FName MissionId) const;

    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    UMissionController* CreateMission(UMissionAsset* MissionAsset);
    virtual FString GetSaveSubsystemName() const override { return TEXT("InteriorSubsystem"); }

private:
    void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
    void HandleInteractCommand(const FOutcomeEventBase& Outcome);
    void HandleSetEnabled(const FOutcomeEventBase& Outcome);
    void HandleSetRange(const FOutcomeEventBase& Outcome);
    void HandleSetTooltip(const FOutcomeEventBase& Outcome);
    void HandleFloorStateSave(const FOutcomeEventBase& Outcome);
    void HandleFloorStateRestore(const FOutcomeEventBase& Outcome);
    void HandleFloorTransition(const FOutcomeEventBase& Outcome);
    void HandleReleaseMissionSnapshot(const FOutcomeEventBase& Outcome);
    void HandleUpdateMissionList(const FOutcomeEventBase& Outcome);
    void HandleCompleteMission(const FOutcomeEventBase& Outcome);
    void StoreCurrentLevel(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason, bool IsSaveAll = false);
    void StoreSnapshot(FName MissionId, FMissionEnvelope Envelope, EJobSpacePolicy Policy, TArray<FEnvelopeChannelEntry>& EndChannels);

    void SubscribeAll();
    void UnsubscribeAll();

    void ApplyPersistentPopulation(const FInteriorFloorKey& FloorKey);
    void SpawnRecords(const TArray<FFloorPopulationRecord>& Records);
    void DestroyRecords(const TArray<FFloorPopulationRecord>& Records);
    AActor* FindActorByItemId(const FGuid& ItemId) const;
    void CopyRecordsForChannel(const TArray<FFloorPopulationRecord>& Source, TArray<FFloorPopulationRecord>& Dest, EEnvelopeChannel Channel);
    void RemoveRecordsForChannel(TArray<FFloorPopulationRecord>& Records, EEnvelopeChannel Channel);

    TMap<FGuid, FInteractItemRecord> RegisteredItems;
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;
    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

    UPROPERTY()
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> SpawnedActorsByInteriorFloor;
    UPROPERTY()
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> DestroyedActorsByInteriorFloor;

    TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>> FloorStateSnapshots;
    TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>> MissionFloorSnapshots;
    mutable TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>> MissionSnapshotQueryCache;

    TMap<FName, FActiveMissionInterior> ActiveMissions;
    UPROPERTY()
    FInteriorFloorKey CurrentKey;
    UPROPERTY()
    UInteriorTransitionPayload* TransitionPayloadCache = nullptr;
    
    UPROPERTY()
    bool bHasPendingSpawn = false;
    UPROPERTY()
    FVector PendingSpawnLocation = FVector::ZeroVector;
    UPROPERTY()
    FRotator PendingSpawnRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FGuid PendingAnchorID;

    FTimerHandle TravelTimerHandle;  // <-- добавлено!

    UPROPERTY()
    UOutcomeConditionAsset* SpawnConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* DespawnConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* PlacementRegisterConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* PlacementUnregisterConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* SetEnabledConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* FloorStateSaveConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* FloorStateRestoreConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* FloorTransitionConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* MissionReleaseConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* UpdateMissionListConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* CompleteMissionConditionAsset = nullptr;

	bool IsLoadingFromSave = false;
	bool IsSaveing = false;

    FDelayedClear DelayedClear;

    FOutcomeHandlerHandle SpawnRegisterHandle;
    FOutcomeHandlerHandle DespawnRegisterHandle;
    FOutcomeHandlerHandle PlacementRegisterHandle;
    FOutcomeHandlerHandle PlacementUnregisterHandle;
    FOutcomeHandlerHandle InteractCommandHandle;
    FOutcomeHandlerHandle SetEnabledHandle;
    FOutcomeHandlerHandle SetRangeHandle;
    FOutcomeHandlerHandle SetTooltipHandle;
    FOutcomeHandlerHandle FloorStateSaveHandle;
    FOutcomeHandlerHandle FloorStateRestoreHandle;
    FOutcomeHandlerHandle FloorTransitionHandle;
    FOutcomeHandlerHandle MissionReleaseHandle;
    FOutcomeHandlerHandle UpdateMissionListHandle;
    FOutcomeHandlerHandle CompleteMissionHandle;

    FDelegateHandle ActorSpawnedHandle;
    FDelegateHandle PostLoadMapHandle;

    void OnPostLoadMap(UWorld* LoadedWorld);

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override;
};