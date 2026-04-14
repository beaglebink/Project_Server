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
#include "InteriorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

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
	bool       bHasRelativeTransform = false; // true — это дочерний актор, восстанавливать через компонент
	TArray<FFloorSavedPropertyEntry>  ActorProperties;
	TArray<FFloorSavedComponentState> ComponentStates;
};

UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
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

	// Legacy monitoring broadcast (kept for tools/debug)
	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorInteractItemRegistrationEvent OnInteractItemUnregistered;

	// Получить список размещённых объектов для конкретного InteriorSet+Floor (runtime)
	// Возвращается копия массива — безопасно для Blueprint/UFunction
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Population")
	TArray<FFloorPopulationRecord> GetPlacedActorsForInteriorFloor(const FGuid& InteriorSetId, const FGuid& FloorId) const;

	// ── Snapshot API (BlueprintCallable, EventBus-driven) ─────────────────

	/** Добавляет снимок состояния акторов этажа в память подсистемы. Не перезаписывает уже сохранённые. */
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
	void SaveFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId);

	/** Восстанавливает состояние акторов этажа из памяти. Возвращает количество восстановленных акторов. */
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
	int32 RestoreFloorActorsState(const FGuid& InteriorSetId, const FGuid& FloorId);

	// Build runtime cache of interior assets (friendly keys -> GUID key)
	// Friendly key format: "<SoftObjectPath>:floor_<FloorIndex>"
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Persistence")
	void BuildAssetIndex();

private:
	// Handler for spawn/despawn interactive objects
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);

	// Handler for floor placement registration (separate from interactive registration)
	void HandlePlacementRegistration(const FOutcomeEventBase& Outcome);

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
	TMap<FInteriorFloorKey, FFloorPopulationBuckets> SpawnedActorsByInteriorFloor;

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

	// Friendly (designer) index: FriendlyKey -> (InteriorSetId, FloorId)
	// FriendlyKey example: "/Game/Props/House1.House1:floor_2"
	TMap<FString, FInteriorFloorKey> FriendlyFloorIndex;

	// Resolve by asset name + floor index
	bool TryResolveFloorKeyByAssetName(const FString& AssetName, int32 FloorIndex, FInteriorFloorKey& OutKey) const;

	// Subscribe/unsubscribe placement-specific handlers
	void SubscribePlacementRegistration();
	void UnsubscribePlacementRegistration();

	// Реализация абстрактного доступа для FInteractiveSubsystemMethods
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
	{
		return RegistrationListeners;
	}
};
