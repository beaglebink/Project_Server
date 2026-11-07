#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "InventorySnapshotTypes.h"
#include "ISaveableSubsystem.h"
#include "InventorySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAcquiredEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

class UInteractiveItemComponent;
class UAC_Container;

USTRUCT()
struct FInventoryInteractItemRecord
{
    GENERATED_BODY()
    UPROPERTY() FGuid ItemId;
    UPROPERTY() TObjectPtr<AActor> OwnerActor = nullptr;
    UPROPERTY() float InteractionRange = 0.f;
};

UCLASS()
class FPSKITALSREFACTORED_API UInventorySubsystem
    : public UGameInstanceSubsystem
    , public FInteractiveSubsystemMethods
    , public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ── ISaveableSubsystem ──
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("InventorySubsystem"); }
    virtual bool GetIsLoadComplete() const override { return IsLoadComplete; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> ItemAcquiredCondition;

    UPROPERTY(BlueprintAssignable, Category = "EventBus|Events") FOnItemAcquiredEvent OnItemAcquired;
    UPROPERTY(BlueprintAssignable, Category = "EventBus|Events") FOnInventoryInteractItemRegistrationEvent OnInteractItemRegistered;
    UPROPERTY(BlueprintAssignable, Category = "EventBus|Events") FOnInventoryInteractItemRegistrationEvent OnInteractItemUnregistered;

    // ── ItemAcquired ──
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeItemAcquired();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeItemAcquired();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition);
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") bool IsItemAcquiredSubscribed() const { return ItemAcquiredHandle.IsValid(); }

    // ── Registration ──
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeRegistration();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeRegistration();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeAll();
    void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
    void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

    // ── InteractCommand ──
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeInteractCommand();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeInteractCommand();

    // ── SetRange / SetTooltip ──
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeSetRange();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeSetRange();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeSetTooltip();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeSetTooltip();

    // ── Player inventory snapshot ──
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InventorySubsystem|Snapshot")
    bool HasSavedInventorySnapshot() const { return bHasSavedInventorySnapshot; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InventorySubsystem|Snapshot")
    FInventorySnapshot GetSavedInventorySnapshot() const { return SavedInventorySnapshot; }

    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeInventorySnapshot();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeInventorySnapshot();

    // ── Containers (world) ──
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Containers") void RegisterLiveContainer(UAC_Container* Container);
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Containers") void UnregisterLiveContainer(UAC_Container* Container);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InventorySubsystem|Containers")
    bool HasContainerSnapshot(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Containers")
    bool GetContainerSnapshot(const FGuid& ItemId, FInventorySnapshot& OutSnapshot) const;

    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void SubscribeContainerSnapshot();
    UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers") void UnsubscribeContainerSnapshot();

private:
    // Handlers
    void HandleItemAcquired(const FOutcomeEventBase& Outcome);
    void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
    void HandleInteractCommand(const FOutcomeEventBase& Outcome);
    void HandleSetRange(const FOutcomeEventBase& Outcome);
    void HandleSetTooltip(const FOutcomeEventBase& Outcome);
    void HandleSaveInventoryRequest(const FOutcomeEventBase& Outcome);
    void HandleRestoreInventoryRequest(const FOutcomeEventBase& Outcome);
    void HandleSaveContainerRequest(const FOutcomeEventBase& Outcome);

    void RefreshPlayerInventorySnapshot();
    void RefreshContainerSnapshotsFromLive();
    void OnPostLoadMap_ClearLoadingFlag(UWorld* LoadedWorld);

    FOutcomeHandlerHandle ItemAcquiredHandle;
    FOutcomeHandlerHandle RegisteredRegisterHandle;
    FOutcomeHandlerHandle UnregisteredRegisterHandle;
    FOutcomeHandlerHandle InteractCommandHandle;
    FOutcomeHandlerHandle SetRangeHandle;
    FOutcomeHandlerHandle SetTooltipHandle;
    FOutcomeHandlerHandle SaveInventoryHandle;
    FOutcomeHandlerHandle RestoreInventoryHandle;
    FOutcomeHandlerHandle SaveContainerHandle;

    UPROPERTY() UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SaveInventoryConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* RestoreInventoryConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SaveContainerConditionAsset = nullptr;

    UPROPERTY() TMap<FGuid, FInventoryInteractItemRecord> RegisteredItems;
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;
    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

    UPROPERTY() FInventorySnapshot SavedInventorySnapshot;
    bool bHasSavedInventorySnapshot = false;

    UPROPERTY() TMap<FGuid, FInventorySnapshot> ContainerSnapshots;
    UPROPERTY() TMap<FGuid, TWeakObjectPtr<UAC_Container>> LiveContainers;

    bool bIsLoadingFromDisk = false;
    FDelegateHandle PostLoadMapClearLoadingHandle;
    bool IsLoadComplete = true;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }
};