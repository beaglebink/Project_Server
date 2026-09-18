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

    // ===== УСЛОВИЯ ДЛЯ ПОДПИСКИ НА СОБЫТИЯ (настраиваются в редакторе) =====

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> WorldStateAddRecordCondition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> WorldStateRemoveRecordCondition;

    // ===== СОБЫТИЕ ДЛЯ BLUEPRINT =====

    UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
    FOnChangingLocationAvailabilityEvent OnChangingLocationAvailability;

    // ===== УПРАВЛЕНИЕ ПОДПИСКАМИ (публичные) =====

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    void UnsubscribeAll();

    // ===== МЕТОДЫ ЧТЕНИЯ СОСТОЯНИЯ (публичные) =====

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldStateSubsystem|State")
    bool HasWorldStateRecord(const FGuid& ItemId, FName ChangeKey) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    bool GetWorldStateRecord(const FGuid& ItemId, FName ChangeKey, FWorldStateRecord& OutRecord) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    TArray<FWorldStateRecord> GetRecordsForItem(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    TArray<FWorldStateRecord> GetRecordsByCategory(EWorldStateChangeCategory Category) const;

    // ===== ISaveableSubsystem =====

    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("WorldStateSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return IsLoadComplete; }

private:
    void SubscribeAllWorldStateEvents();
    UOutcomeConditionAsset* CreateSimpleWorldStateCondition(EOutcomeWorldState WorldStateType);

    // ---- ОБРАБОТЧИКИ СОБЫТИЙ (приватные) ----

    void HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome);
    void HandleRemoveWorldStateRecord(const FOutcomeEventBase& Outcome);

    // Обработчик завершения загрузки уровня (публикуется InteriorSubsystem).
    // Вызывается после того, как InteriorSubsystem восстановил свои снапшоты.
    void HandleLevelLoaded(const FOutcomeEventBase& Outcome);

    // Обработчик появления актора — применяет к нему уже накопленные записи.
    // Нужен для late-spawn и стриминга.
    void HandleActorSpawned(AActor* SpawnedActor);

    // ---- ХЕНДЛЫ ПОДПИСОК ----

    FOutcomeHandlerHandle WorldStateRecordHandle;
    FOutcomeHandlerHandle WorldStateRecordRemoveHandle;
    FOutcomeHandlerHandle LevelLoadedHandle;

    UPROPERTY()
    UOutcomeConditionAsset* LevelLoadedConditionAsset = nullptr;

    // ---- ПОДПИСКА НА СПАВН АКТОРОВ ----

    FDelegateHandle ActorSpawnedHandle;
    TWeakObjectPtr<UWorld> SubscribedWorld;

    void SubscribeToActorSpawned();
    void UnsubscribeFromActorSpawned();

    // ---- ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ СОСТОЯНИЯ ----

    void SetWorldStateRecord(const FWorldStateRecord& Record);
    void RemoveWorldStateRecord(const FGuid& ItemId, FName ChangeKey);
    void ApplyRecordsToWorld();
    void ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const;

    // ---- ПОИСК АКТОРОВ ПО ItemId ----

    AActor* FindActorByItemId(const FGuid& ItemId) const;
    void BuildActorIndex(TMap<FGuid, AActor*>& OutIndex) const;

    // ---- СЛУШАТЕЛИ PER-ITEM ----

    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }

    // ---- ХРАНИЛИЩЕ ЗАПИСЕЙ ----

    TMap<FGuid, TMap<FName, FWorldStateRecord>> WorldStateRecords;

    bool IsLoadComplete = true;
};