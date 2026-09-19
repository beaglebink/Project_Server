#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "../SaveGame/ISaveableSubsystem.h"
#include "WorldStateTypes.h"
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

    // ===== МЕТОДЫ ЧТЕНИЯ =====
    // bIncludePendingRemoval:
    //   false (по умолчанию) — записи с bPendingRemoval = true игнорируются
    //   true                 — такие записи включаются в результат

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

    // ===== C++-геттеры (без рефлексии) =====
    const FWorldStateRecord* FindWorldStateRecord(FName FactId, bool bIncludePendingRemoval = false) const;

    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("WorldStateSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return IsLoadComplete; }

private:
    void SubscribeAllWorldStateEvents();
    UOutcomeConditionAsset* CreateSimpleWorldStateCondition(EOutcomeWorldState WorldStateType);

    // ---- ОБРАБОТЧИКИ ----
    void HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome);
    void HandleRemoveWorldStateRecord(const FOutcomeEventBase& Outcome);
    void HandleLevelLoaded(const FOutcomeEventBase& Outcome);
    void HandleActorSpawned(AActor* SpawnedActor);

    // ---- ХЕНДЛЫ ПОДПИСОК ----
    FOutcomeHandlerHandle WorldStateRecordHandle;
    FOutcomeHandlerHandle WorldStateRecordRemoveHandle;
    FOutcomeHandlerHandle LevelLoadedHandle;

    UPROPERTY()
    UOutcomeConditionAsset* LevelLoadedConditionAsset = nullptr;

    // ---- ПОДПИСКА НА СПАВН ----
    FDelegateHandle ActorSpawnedHandle;
    TWeakObjectPtr<UWorld> SubscribedWorld;

    void SubscribeToActorSpawned();
    void UnsubscribeFromActorSpawned();

    // ---- ИЗМЕНЕНИЕ СОСТОЯНИЯ ----
    void SetWorldStateRecord(const FWorldStateRecord& Record);
    void RemoveWorldStateRecord(FName FactId);
    void ApplyRecordsToWorld();
    void ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const;

    void CaptureOriginalValueIfMissing(FName FactId, AActor* Actor);
    bool TryFinalizePendingRemoval(FName FactId, AActor* Actor);

    // Публикует событие факта с полным снимком записи.
    //   PreviousValue — для Changed (пустая строка для Added/Removed).
    //   RestoredValue / bHasRestoredValue — для Removed (для Added/Changed
    //   передаются пустая строка и false).
    void PublishFactEvent(FName FactId, const FWorldStateRecord& RecordSnapshot, EOutcomeWorldState EventType, const FString& PreviousValue = FString(), const FString& RestoredValue = FString(), bool bHasRestoredValue = false) const;

    // ---- ПОИСК ----
    AActor* FindActorByItemId(const FGuid& ItemId) const;
    void BuildActorIndex(TMap<FGuid, AActor*>& OutIndex) const;
    UActorComponent* FindComponentByStableName(AActor* Actor, FName ComponentName) const;

    FProperty* ResolveTargetProperty(AActor* Actor, const FWorldStateRecord& Record, UObject*& OutTargetObject) const;
    bool TryReadPropertyValue(AActor* Actor, const FWorldStateRecord& Record, FString& OutValue) const;
    bool WritePropertyValue(AActor* Actor, const FWorldStateRecord& Record, const FString& Value, bool bIsRestore) const;
    void InvokeReactionFunction(UObject* Target, const FWorldStateRecord& Record) const;

    // ---- СЛУШАТЕЛИ PER-ITEM ----
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }

    // ---- ХРАНИЛИЩЕ ----
    // Ключ — FactId.
    TMap<FName, FWorldStateRecord> WorldStateRecords;

    bool IsLoadComplete = true;
};