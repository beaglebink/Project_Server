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
#include <SpawnGroupAsset.h>
#include "InteriorCheckTypes.h"
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
    FString ActorName;
    FGuid      ItemId;
    EFloorActorType ActorType;
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
    int32 MissionStep = 0;

    UPROPERTY()
    TObjectPtr<UMissionController> Controller;
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
    void SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason);
    void SaveFloorActorsStateComplete(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason);
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

    // Подсчёт акторов на текущем этаже по типу подсчёта и фильтру типов
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetActorCountForCurrentFloor(EInteriorActorCountType CountType, const TArray<EFloorActorType>& FilterTypes) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem")
    FInteriorFloorKey GetCurrentKey() const { return CurrentKey; }

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedActorCountForCurrentFloor(TSubclassOf<AActor> ActorClass, EFloorActorType ActorType) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedTagCount(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedTagCountForCurrentFloor(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetSpawnedActorCountForCurrentFloor(TSubclassOf<AActor> ActorClass, EFloorActorType ActorType) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetSpawnedTagCountForCurrentFloor(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedSpawnedActorCountForCurrentFloor(TSubclassOf<AActor> ActorClass, EFloorActorType ActorType) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedOriginalActorCountForCurrentFloor(TSubclassOf<AActor> ActorClass, EFloorActorType ActorType) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedSpawnedTagCountForCurrentFloor(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetDestroyedOriginalTagCountForCurrentFloor(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetRemainingSpawnedActorCountForCurrentFloor(TSubclassOf<AActor> ActorClass, EFloorActorType ActorType) const;

    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetRemainingSpawnedTagCountForCurrentFloor(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    // Получить количество оставшихся оригинальных объектов (не заспавненных) с фильтром по классу и типу
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetRemainingOriginalActorCountForCurrentFloor(TSubclassOf<AActor> ActorClass, EFloorActorType ActorType) const;

    // Получить количество оставшихся оригинальных объектов (не заспавненных) с тегом
    UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Check")
    int32 GetRemainingOriginalTagCountForCurrentFloor(ETagType TagType, const FName& TextTag, const FGameplayTag& GameplayTag) const;

    // Public методы для доступа к картам (возвращаем const ссылки)
    const TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& GetMissionSpawnedActors() const { return MissionSpawnedActorsByInteriorFloor; }
    const TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>>& GetMissionDestroyedActors() const { return MissionDestroyedActorsByInteriorFloor; }

    void HandlePlacementRegistration(const FOutcomeEventBase& Outcome);

    bool IsCurrentWorldMissionConst(const FMissionEnvelope& Envelope);

    void RestoreMissionFloorState(FName MissionId, int32 CurrentMissionStep, const FInteriorFloorKey& FloorKey);
    void FloorSpawnActorsFromType(TArray<FFloorPopulationRecord>& Array, const FMissionEnvelope& Envelope, FInteriorFloorKey Key);
    void SpawnMissionActorsFromCurrentFloor(FName MissionId);
    void SpawnMissionRecords(const TArray<FFloorPopulationRecord>& Records);
    void DestroyMissionRecords(const TArray<FFloorPopulationRecord>& Records);
    void RestoreSpawnedActorsForCurrentFloor(const FMissionEnvelope& Envelope);
    void ReleaseMissionSnapshot(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, bool bIsCompletion = false);

    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    UMissionController* CreateMission(UMissionAsset* MissionAsset);
    virtual FString GetSaveSubsystemName() const override { return TEXT("InteriorSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return IsLoadComplete; }
    const FInteriorFloorKey& GetCurrentFloorKey() const { return CurrentKey; }

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
    void StoreCurrentLevel(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason);
    void StoreCurrentLevelComplete(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason);

    void ApplySpawnGroupPolicy(const FSpawnGroupId& GroupId, EChannelPolicy Policy, EMissionEndReason Reason);

    void SubscribeAll();
    void UnsubscribeAll();

    void SpawnRecords(const TArray<FFloorPopulationRecord>& Records);
    void DestroyRecords(const TArray<FFloorPopulationRecord>& Records);
    AActor* FindActorByItemId(const FGuid& ItemId) const;
    void RemoveRecordsForChannel(TArray<FFloorPopulationRecord>& Records, EEnvelopeChannel Channel);

    TMap<FGuid, FInteractItemRecord> RegisteredItems;
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;
    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

    UPROPERTY()
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> SpawnedActorsByInteriorFloor;
    UPROPERTY()
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> DestroyedActorsByInteriorFloor;

    UPROPERTY()
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> SpawnedActorsByInteriorFloorTemp;
    UPROPERTY()
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> DestroyedActorsByInteriorFloorTemp;

    TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>> MissionSpawnedActorsByInteriorFloor;

    TMap<FInteriorFloorKey, TMap<FName, FFloorPopulationBuckets>> MissionDestroyedActorsByInteriorFloor;

    TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>> FloorStateSnapshots;
    TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>> MissionFloorSnapshots;

    mutable TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>> MissionSnapshotQueryCache;

    TMap<FInteriorFloorKey, FFloorPopulationBuckets> AllSpawnedActorsByInteriorFloor;
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> AllDestroyedActorsByInteriorFloor;
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> AllDestroyedSpawnedActorsByInteriorFloor;
    TMap<FInteriorFloorKey, FFloorPopulationBuckets> AllDestroyedOriginalActorsByInteriorFloor;

    UPROPERTY()
    TMap<FName, FActiveMissionInterior> ActiveMissions;

    UPROPERTY()
    FInteriorFloorKey CurrentKey;
    UPROPERTY()
    UInteriorTransitionPayload* TransitionPayloadCache = nullptr;

    UPROPERTY()
    TArray<AActor*> TestAllActors;

    UPROPERTY()
    bool bHasPendingSpawn = false;
    UPROPERTY()
    FVector PendingSpawnLocation = FVector::ZeroVector;
    UPROPERTY()
    FRotator PendingSpawnRotation = FRotator::ZeroRotator;
    UPROPERTY()
    FGuid PendingAnchorID;

    FTimerHandle TravelTimerHandle;

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

    /** Перестраивает вспомогательные карты AllSpawned / AllDestroyed для текущего этажа,
     *  синхронизируя их с фактическим состоянием мира и данными из SpawnedActorsByInteriorFloor / DestroyedActorsByInteriorFloor. */
    void RebuildPopulationMapsForCurrentFloor();

    bool IsLoadComplete = true;
	bool IsPostLoadMapComplete = false; 
};