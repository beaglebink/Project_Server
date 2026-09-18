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
    // Записи с bPendingRemoval = true игнорируются.

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldStateSubsystem|State")
    bool HasWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey) const;

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
    bool GetWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey, FWorldStateRecord& OutRecord) const;

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

    // ---- ОБРАБОТЧИКИ СОБЫТИЙ ----
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

    // ---- ПОДПИСКА НА СПАВН АКТОРОВ ----
    FDelegateHandle ActorSpawnedHandle;
    TWeakObjectPtr<UWorld> SubscribedWorld;

    void SubscribeToActorSpawned();
    void UnsubscribeFromActorSpawned();

    // ---- ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ СОСТОЯНИЯ ----
    void SetWorldStateRecord(const FWorldStateRecord& Record);
    void RemoveWorldStateRecord(const FGuid& ItemId, FName ComponentName, FName ChangeKey);
    void ApplyRecordsToWorld();
    void ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const;

    // Захватывает OriginalValue, если он ещё не захвачен.
    void CaptureOriginalValueIfMissing(const FWorldStateKey& Key, AActor* Actor);

    // Пытается финализировать отложенное удаление записи для указанного актёра.
    // Возвращает true, если запись была удалена из карты.
    bool TryFinalizePendingRemoval(const FWorldStateKey& Key, AActor* Actor);

    // ---- ПОИСК АКТОРОВ / КОМПОНЕНТОВ / СВОЙСТВ ----
    AActor* FindActorByItemId(const FGuid& ItemId) const;
    void BuildActorIndex(TMap<FGuid, AActor*>& OutIndex) const;
    UActorComponent* FindComponentByStableName(AActor* Actor, FName ComponentName) const;

    FProperty* ResolveTargetProperty(AActor* Actor, const FWorldStateRecord& Record, UObject*& OutTargetObject) const;
    bool TryReadPropertyValue(AActor* Actor, const FWorldStateRecord& Record, FString& OutValue) const;
    bool WritePropertyValue(AActor* Actor, const FWorldStateRecord& Record, const FString& Value, bool bIsRestore) const;

    // Вызывает UFUNCTION без параметров с именем Record.ReactionFunctionName
    // на целевом объекте. Если имя не задано или функция не найдена — тихо
    // ничего не делает (Warning в лог при невалидном имени).
    void InvokeReactionFunction(UObject* Target, const FWorldStateRecord& Record) const;

    // ---- СЛУШАТЕЛИ PER-ITEM ----
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }

    // ---- ХРАНИЛИЩЕ ЗАПИСЕЙ ----
    TMap<FWorldStateKey, FWorldStateRecord> WorldStateRecords;

    bool IsLoadComplete = true;
};