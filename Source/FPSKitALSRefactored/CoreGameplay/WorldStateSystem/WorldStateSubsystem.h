#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "../SaveGame/ISaveableSubsystem.h"
#include "WorldStateTypes.h"
#include "Engine/World.h"
#include "WorldStateSubsystem.generated.h"

class UInteractiveItemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChangingLocationAvailabilityEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UWorldStateSubsystem : public UGameInstanceSubsystem,
    public FInteractiveSubsystemMethods,
    public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> WorldStateAddRecordCondition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> WorldStateRemoveRecordCondition;

    UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
    FOnChangingLocationAvailabilityEvent OnChangingLocationAvailability;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    void UnsubscribeAll();

    // ===== READ METHODS =====
    // bIncludePendingRemoval:
    //   false (default) — records with bPendingRemoval = true are ignored
    //   true            — such records are included in the result

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldStateSubsystem|State")
    bool HasWorldStateRecord(FName FactId, bool bIncludePendingRemoval = false) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    bool GetWorldStateRecord(FName FactId, bool bIncludePendingRemoval, FWorldStateRecord& OutRecord) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldStateSubsystem|State")
    FWorldStateRecord GetWorldStateRecordOrDefault(FName FactId, bool bIncludePendingRemoval = false) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    TArray<FWorldStateRecord> GetRecordsForItem(const FGuid& ItemId, bool bIncludePendingRemoval = false) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    TArray<FWorldStateRecord> GetRecordsByCategory(EWorldStateChangeCategory Category, bool bIncludePendingRemoval = false) const;

    // Детерминированный FactId для факта «интерактивность» одного актёра.
    // Можно вызывать снаружи, чтобы вручную снять override через
    // UWorldStateRecordRemovePayload с этим FactId.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldStateSubsystem|Interact")
    static FName MakeInteractEnabledFactId(const FGuid& ActorPersistentId);

    // ===== C++ getters (without reflection) =====
    const FWorldStateRecord* FindWorldStateRecord(FName FactId, bool bIncludePendingRemoval = false) const;

    // ===== ISaveableSubsystem =====
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("WorldStateSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return IsLoadComplete; }

protected:
    void OnWorldBeginPlay(UWorld& InWorld);

private:
    // ---- SETUP ----
    void SubscribeAllWorldStateEvents();
    UOutcomeConditionAsset* CreateSimpleWorldStateCondition(EOutcomeWorldState WorldStateType);

    // ---- HANDLERS ----
    void HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome);
    void HandleRemoveWorldStateRecord(const FOutcomeEventBase& Outcome);
    void HandleLevelLoaded(const FOutcomeEventBase& Outcome);
    void HandleActorSpawned(AActor* SpawnedActor);

    // ---- SUBSCRIPTION HANDLES ----
    FOutcomeHandlerHandle WorldStateRecordHandle;
    FOutcomeHandlerHandle WorldStateRecordRemoveHandle;
    FOutcomeHandlerHandle LevelLoadedHandle;

    UPROPERTY()
    UOutcomeConditionAsset* LevelLoadedConditionAsset = nullptr;

    FDelegateHandle ActorSpawnedHandle;
    TWeakObjectPtr<UWorld> SubscribedWorld;

    void SubscribeToActorSpawned();
    void UnsubscribeFromActorSpawned();

    // ---- STATE MUTATION ----
    void SetWorldStateRecord(const FWorldStateRecord& Record);
    void RemoveWorldStateRecord(FName FactId);
    void ApplyRecordsToWorld();
    void ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const;

    void CaptureOriginalValueIfMissing(FName FactId, AActor* Actor);
    bool TryFinalizePendingRemoval(FName FactId, AActor* Actor);

    // Publishes a fact event with a full snapshot of the record.
    //   PreviousValue — for Changed (empty string for Added/Removed).
    //   RestoredValue / bHasRestoredValue — for Removed.
    void PublishFactEvent(
        FName FactId,
        const FWorldStateRecord& RecordSnapshot,
        EOutcomeWorldState EventType,
        const FString& PreviousValue = FString(),
        const FString& RestoredValue = FString(),
        bool bHasRestoredValue = false) const;

    // ---- LOOKUP ----
    AActor* FindActorByItemId(const FGuid& ItemId) const;
    void BuildActorIndex(TMap<FGuid, AActor*>& OutIndex) const;
    UActorComponent* FindComponentByStableName(AActor* Actor, FName ComponentName) const;

    FProperty* ResolveTargetProperty(AActor* Actor, const FWorldStateRecord& Record, UObject*& OutTargetObject) const;
    bool TryReadPropertyValue(AActor* Actor, const FWorldStateRecord& Record, FString& OutValue) const;
    bool WritePropertyValue(AActor* Actor, const FWorldStateRecord& Record, const FString& Value, bool bIsRestore) const;
    void InvokeReactionFunction(UObject* Target, const FWorldStateRecord& Record) const;

    // ---- PER-ITEM LISTENERS ----
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }

    // ---- STORAGE ----
    // Key — FactId.
    TMap<FName, FWorldStateRecord> WorldStateRecords;

    bool IsLoadComplete = true;

    FDelegateHandle WorldBeginPlayHandle;
    bool bDeferredFirstReevaluate = false;

    // ---- InteractSetEnabled subscription ----
    void SubscribeInteractSetEnabled();
    void HandleInteractSetEnabled(const FOutcomeEventBase& Outcome);

    TArray<FOutcomeHandlerHandle> InteractSetEnabledHandles;
    UPROPERTY()
    TArray<TObjectPtr<UOutcomeConditionAsset>> InteractSetEnabledConditions;

    // Маркер, который в FWorldStateRecord::ComponentName говорит:
    // "резолви первый UInteractiveItemComponent на актёре".
    static const FName InteractComponentMarker;
    static const FName InteractEnabledPropertyName;
    static const FName InteractReactionFuncName;

    AActor* FindActorByRuntimeInteractId(const FGuid& RuntimeItemId) const;
};