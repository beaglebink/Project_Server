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

// Forward-declare UMissionController to avoid including its .cpp in header and to break multiple-definition issues.
class UMissionController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

/** Делегат завершения перехода. bSuccess = true если персонаж телепортирован в позицию якоря. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnTransitionCompleted, bool, bSuccess, FLocationAnchorLink, DestinationLink, bool, IsTravel);

/**
 * Делегат, вызываемый перед уходом с этажа.
 * SourceFloor — ассет этажа, с которого выполняется переход (может быть null если не задан).
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloorExiting, TSoftObjectPtr<UFloorAsset>, SourceFloor);

class UInteractiveItemComponent;

// Record stored for each registered interactive item
struct FInteractItemRecord
{
	FGuid ItemId;
	EInteractiveSubsystem SubsystemType;
	float InteractionRange;
	FText DefaultTooltip;
	TWeakObjectPtr<AActor> OwnerActor;
};

// ─────────────────────────────────────────────────────────────────────────
// In-memory snapshot structs (no UPROPERTY needed — not serialized to disk)
// ─────────────────────────────────────────────────────────────────────────

struct FFloorSavedPropertyEntry
{
	FName  PropertyName;
	FString ValueText;   // ExportText result
};

struct FFloorSavedComponentState
{
	FName ComponentName;       // component FName for matching
	FName ComponentClassName;  // fallback class match
	bool  bWasActive = true;   // UActorComponent::IsActive() at snapshot time
	// Attachment info (for SceneComponent descendants)
	bool  bWasAttached = false;
	FName AttachParentName;    // parent component FName (if attached to a component on same actor)
	FName AttachSocketName;    // socket name used for AttachToComponent

	// Transforms for component
	FTransform RelativeTransform; // component relative transform (preferred for attached children)
	FTransform WorldTransform;    // world transform (fallback)
	bool      bHasRelativeTransform = false;
	bool      bHasWorldTransform = true;

	// Physics backup (valid for UPrimitiveComponent)
	bool    bWasSimulatingPhysics = false;
	FVector SavedLinearVelocity = FVector::ZeroVector;
	FVector SavedAngularVelocityDeg = FVector::ZeroVector;
	TArray<FFloorSavedPropertyEntry> Properties;
};

struct FFloorSavedActorState
{
	FGuid      ItemId;
	FTransform ActorTransform;        // World transform (для независимых акторов)
	FTransform RelativeTransform;     // Relative transform компонента (для Child Actor, если есть)
	bool       bHasRelativeTransform = false; // true — это дочерний актор, восстанавлиать через компонент
	TArray<FFloorSavedPropertyEntry>  ActorProperties;
	TArray<FFloorSavedComponentState> ComponentStates;
};

USTRUCT()
struct FActiveMissionInterior
{
	GENERATED_BODY()

	// Идентификатор миссии (имя ассета)
	UPROPERTY()
	FName MissionId;

	UPROPERTY()
	int32 MissionStep;

	UPROPERTY()
	TObjectPtr<UMissionController> Controller;
};	



UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods, public ISaveableSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	void SubscribeToSpawnActor();
	void UnsubscribeFromSpawnActor();
	virtual void Deinitialize() override;

	// ===== Interaction registration =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeInteractionRegistration();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeInteractionRegistration();

	// ===== Interact command handling (EventBus) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeInteractCommand();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeInteractCommand();

	// ===== SetEnabled command (вкл/выкл интерактивного компонента) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeSetEnabled();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeSetEnabled();

	// ===== SetRange command (установка диапазона взаимодействия) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeSetRange();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeSetRange();

	// ===== SetTooltip command (установка подсказки взаимодействия) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeSetTooltip();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeSetTooltip();

	// Per-item listener API (interior): only the matching component gets notified
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

	void OnActorSpawned(AActor* SpawnedActor);

	void TryRegisterActor(AActor* SpawnedActor, int32 AttemptsLeft);

	// Legacy monitoring broadcast (kept for tools/debug)
	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorInteractItemRegistrationEvent OnInteractItemUnregistered;

	// ── Делегат завершения перехода ────────────────────────────────────────
	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnTransitionCompleted OnTransitionCompleted;

	/**
	 * Вызывается перед уходом с этажа (до SeamlessTravel или телепорта).
	 * Используйте для выполнения.actions на исходном этаже (сохранение состояния, анимации и т.д.).
	 * SourceFloor — ассет этажа, с которого уходим (null если не был указан в дескрипторе).
	 */
	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnFloorExiting OnFloorExiting;

	// Получить список размещённых объектов для конкретного InteriorSet+Floor (runtime)
	// Возвращается копия массива — безопасно для Blueprint/UFunction
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Population")
	TArray<FFloorPopulationRecord> GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const;

	// ── Snapshot API (BlueprintCallable, EventBus-driven) ─────────────────

	/** Сохраняет снимок состояния акторов этажа.
	 *  Если MissionId задан — сохраняет в_slot миссии, иначе в общий слот (NAME_None). */
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
	void SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionId, EMissionEndReason Reason);

	/** Восстанавливает состояние акторов этажа из памяти. Возвращает количество восстановленных акторов. */
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
	int32 RestoreFloorActorsState(const FGuid& InteriorSetId, int32 CurrentMissionStep, const FGuid& FloorId);

	int32 RestoreFromSnapshotArray(UWorld* W, const TArray<FFloorSavedActorState>& Snapshots, const FGuid& InteriorSetId, const FGuid& FloorId, const FMissionEnvelope Envelope);

	// Pending spawn transform API (public для доступа из GameMode)
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
	void SetPendingSpawnTransform(const FVector& Location, const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
	void GetPendingSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const;

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
	void ClearPendingSpawnTransform();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem|Transition")
	bool HasPendingSpawnTransform() const;

	// ── Pending anchor ID (для поиска ALocationAnchorActor после загрузки карты) ──
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
	void SetPendingAnchorID(const FGuid& AnchorID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem|Transition")
	FGuid GetPendingAnchorID() const;

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
	void ClearPendingAnchorID();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteriorSubsystem|Transition")
	bool HasPendingAnchorID() const;

	/**
	 * Ищет ALocationAnchorActor по AnchorID на текущей карте и телепортирует персонажа.
	 * Вызывается после загрузки карты (из GameMode или PostSeamlessTravel).
	 * Возвращает true если якорь найден и персонаж телепортирован.
	 */
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Transition")
	bool TeleportToAnchor(const FGuid& AnchorID);

	// Subscribe/unsubscribe placement-specific handlers (public declarations required by cpp)
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Placement")
	void SubscribePlacementRegistration();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Placement")
	void UnsubscribePlacementRegistration();

	// Handler for floor placement registration (moved to public to avoid access issues in delegate binding)
	UFUNCTION()
	void HandlePlacementRegistration(const FOutcomeEventBase& Outcome);

	// ── Mission Snapshot API ────────────────────────────────────────────────
	// Сохранить состояние конкретного этажа под ключом миссии.
	// Используется MissionSubsystem при активации миссии.
	void SaveMissionFloorState(FName MissionId, const FInteriorFloorKey& FloorKey);

	// Восстановить состояние этажа из mission-snapshot.
	// Применяется перед показом этажа (re-enter) в рамках активной миссии.
	void RestoreMissionFloorState(FName MissionId, int32 CurrentMissionStep, const FInteriorFloorKey& FloorKey);

	void SpawnFromType(TArray<FFloorPopulationRecord>& Array, const FMissionEnvelope& Envelope);

	void RestoreSpawnedActorsForCurrentFloor(const FMissionEnvelope& Envelope);

	// Освободить mission-snapshot после завершения миссии.
	// Policy определяет что делать с накопленными изменениями.
	void ReleaseMissionSnapshot(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, bool bIsCompletion = false);

	// Получить список снимков всех этажей для конкретной миссии (для сохранения на диск)
	const TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>>* GetMissionSnapshots(FName MissionId) const;

	// ===== ISaveableSubsystem =====
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
	UMissionController* CreateMission(UMissionAsset* MissionAsset);
	virtual FString GetSaveSubsystemName() const override { return TEXT("InteriorSubsystem"); }

private:
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);

	// Runtime condition assets to keep alive
	UPROPERTY()
	UOutcomeConditionAsset* SpawnConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* DespawnConditionAsset = nullptr;
	// Condition assets/handles for floor placement registration
	UPROPERTY()
	UOutcomeConditionAsset* PlacementRegisterConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* PlacementUnregisterConditionAsset = nullptr;

	// Handles for interaction registration
	FOutcomeHandlerHandle SpawnRegisterHandle;
	FOutcomeHandlerHandle DespawnRegisterHandle;
	// Handles for placement registration
	FOutcomeHandlerHandle PlacementRegisterHandle;
	FOutcomeHandlerHandle PlacementUnregisterHandle;

	// Registry of interactive items
	TMap<FGuid, FInteractItemRecord> RegisteredItems;

	// Per-item listeners: only invoked for matching ItemId
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	// Cached EventBus subsystem for outcome publishing
	UPROPERTY()
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

	// Helpers to subscribe/unsubscribe groups
	void SubscribeAll();
	void UnsubscribeAll();

	// Optional debug helper
	void EvaluateConditions();

private:
	// Interact command handler and runtime condition/handle
	void HandleInteractCommand(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;

	FOutcomeHandlerHandle InteractCommandHandle;

	// SetEnabled command handler and runtime condition/handle
	void HandleSetEnabled(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* SetEnabledConditionAsset = nullptr;

	FOutcomeHandlerHandle SetEnabledHandle;

	// SetRange command handler and runtime condition/handle
	void HandleSetRange(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;

	FOutcomeHandlerHandle SetRangeHandle;

	// SetTooltip command handler and runtime condition/handle
	void HandleSetTooltip(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;

	FOutcomeHandlerHandle SetTooltipHandle;

	// Runtime map: (InteriorSetId, FloorId) -> placed actors
	UPROPERTY()
	TMap<FInteriorFloorKey, FFloorPopulationBuckets> SpawnedActorsByInteriorFloor;

	//UPROPERTY()
	TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>> SpawnedActorsSnapshots;

	UPROPERTY()
	TMap<FInteriorFloorKey, FFloorPopulationBuckets> DestroyedActorsByInteriorFloor;

	// ── In-memory snapshot storage (appended per floor, never overwritten) ─
	// Key = FInteriorFloorKey; value = list of actor snapshots for that floor.
	TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>> FloorStateSnapshots;

	// EventBus handlers for snapshot commands
	void HandleFloorStateSave   (const FOutcomeEventBase& Outcome);
	void HandleFloorStateRestore(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* FloorStateSaveConditionAsset    = nullptr;
	UPROPERTY()
	UOutcomeConditionAsset* FloorStateRestoreConditionAsset = nullptr;

	FOutcomeHandlerHandle FloorStateSaveHandle;
	FOutcomeHandlerHandle FloorStateRestoreHandle;

	// Floor transition (SeamlessTravel + anchor placement)
	void HandleFloorTransition(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* FloorTransitionConditionAsset = nullptr;

	FOutcomeHandlerHandle FloorTransitionHandle;

	FTimerHandle TravelTimerHandle;

private:
	// Resolve by asset name + floor index
	bool TryResolveFloorKeyByAssetName(const FString& AssetName, int32 FloorIndex, FInteriorFloorKey& OutKey) const;

	// pending transform storage (GameInstance lifetime)
	UPROPERTY()
	bool bHasPendingSpawn = false;

	UPROPERTY()
	FVector PendingSpawnLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator PendingSpawnRotation = FRotator::ZeroRotator;

	// pending anchor ID (для поиска актора после загрузки карты)
	UPROPERTY()
	FGuid PendingAnchorID;


	TMap<FName, FActiveMissionInterior> ActiveMissions;

	FDelegateHandle ActorSpawnedHandle;

	UPROPERTY()	
	FInteriorFloorKey CurrentKey;

protected:
	// Реализация абстрактного доступа для FInteractiveSubsystemMethods
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override;

	// Friendly (designer) index: FriendlyKey -> (InteriorSetId, FloorId)
	UPROPERTY()
	TMap<FString, FInteriorFloorKey> FriendlyFloorIndex;

private:
	// ── Mission floor snapshots ────────────────────────────────────────────────
	// Теперь: внешний ключ — FInteriorFloorKey (InteriorSetId + FloorId).
	// Внутренний map: MissionId -> список снимков акторов для этой миссии на данном этаже.
	// Это ускоряет поиск по этажу при восстановлении после загрузки.
	TMap<FInteriorFloorKey, TMap<FName, TArray<FFloorSavedActorState>>> MissionFloorSnapshots;

	// Кеш результаты запроса GetMissionSnapshots (возвращаем указатель на этот кеш)
	mutable TMap<FInteriorFloorKey, TArray<FFloorSavedActorState>> MissionSnapshotQueryCache;

	// Обработчик завершения загрузки карты (после SeamlessTravel)
	void OnPostLoadMap(UWorld* LoadedWorld);
	FDelegateHandle PostLoadMapHandle;

	UPROPERTY()
	UInteriorTransitionPayload* TransitionPayloadCache = nullptr;

	// ----- Mission release via EventBus -----
	UPROPERTY()
	UOutcomeConditionAsset* MissionReleaseConditionAsset = nullptr;

	FOutcomeHandlerHandle MissionReleaseHandle;

	// Handler invoked when a ReleaseMissionSnapshot command arrives via EventBus
	void HandleReleaseMissionSnapshot(const FOutcomeEventBase& Outcome);

	// ----- Update mission list via EventBus -----
	UPROPERTY()
	UOutcomeConditionAsset* UpdateMissionListConditionAsset = nullptr;

	FOutcomeHandlerHandle UpdateMissionListHandle;

	void HandleUpdateMissionList(const FOutcomeEventBase& Outcome);

	// ----- Complete mssion via EventBus -----
	UPROPERTY()
	UOutcomeConditionAsset* CompleteMissionConditionAsset = nullptr;

	FOutcomeHandlerHandle CompleteMissionHandle;

	void HandleCompleteMission(const FOutcomeEventBase& Outcome);
	void StoreCurrentLevel(FMissionEnvelope Envelope, FName MissionId, EMissionEndReason EndReason);
	void StoreSnapshot(FName MissionId, FMissionEnvelope Envelope, EJobSpacePolicy Policy, TArray<FEnvelopeChannelEntry>& EndChannels);
};
