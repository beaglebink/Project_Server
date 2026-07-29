#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "SpawnGroupAsset.h"
#include "SpawnGroupTypes.h"
#include "ISaveableSubsystem.h"
#include "SpawnGroupSubsystem.generated.h"

class ASpawnGroupSpawner;

UCLASS()
class FPSKITALSREFACTORED_API USpawnGroupSubsystem : public UGameInstanceSubsystem, public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    // ----- Жизненный цикл -----
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ----- Подписка на события EventBus (для внешнего использования) -----
    // Эти методы не являются UFUNCTION, так как они используются для подписки/отписки.
    // Для обратной совместимости оставляем их публичными, но в основном они вызываются из Initialize/Deinitialize.

    // ----- Обработчик события GhostCaptured (доступен для BP? нет) -----
    // Обработчик объявлен как public, но без UFUNCTION – его можно вызвать только из C++.
    void HandleGhostCaptured(const FOutcomeEventBase& Outcome);

    // ----- ISaveableSubsystem -----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("SpawnGroupSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

    // =========================================================================
    // Статистика по конкретной группе (по ItemId спавнера)
    // =========================================================================

    // ---- Общие ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetTotalSpawnedCount(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCount(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCount(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCount(const FGuid& ItemId) const;

    // ---- По классам (одиночный класс) ----
    /*
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const;
    */
    // ---- По классам (массив классов) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByType(const FGuid& ItemId, const TArray<TSubclassOf<AActor>>& ActorClasses) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByType(const FGuid& ItemId, const TArray<TSubclassOf<AActor>>& ActorClasses) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByType(const FGuid& ItemId, const TArray<TSubclassOf<AActor>>& ActorClasses) const;

    // ---- По текстовым тегам (массив) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const;

    // ---- По GameplayTag (массив) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const;

    // ---- Статус группы ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    ESpawnGroupStatus GetGroupStatus(const FGuid& ItemId) const;

    // =========================================================================
    // Статистика по текущему этажу (агрегирует все группы на этаже)
    // =========================================================================

    // ---- Общие ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetTotalSpawnedCountForCurrentFloor() const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountForCurrentFloor() const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountForCurrentFloor() const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountForCurrentFloor() const;

    // ---- По классам (одиночный класс) ----
    /*
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const;
    */
    // ---- По классам (массив классов) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTypeForCurrentFloor(const TArray<TSubclassOf<AActor>>& ActorClasses) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTypeForCurrentFloor(const TArray<TSubclassOf<AActor>>& ActorClasses) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByTypeForCurrentFloor(const TArray<TSubclassOf<AActor>>& ActorClasses) const;

    // ---- По текстовым тегам (массив) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const;

    // ---- По GameplayTag (массив) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetCapturedCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const;

private:
    // ----- Подписка на события EventBus -----
    void SubscribeEvents();
    void UnsubscribeEvents();

    // ----- Вспомогательные методы -----
    FInteriorFloorKey GetFloorKeyFromSpawner(ASpawnGroupSpawner* Spawner) const;

    // ----- Обработчики событий -----
    void HandleSpawnGroupRegister(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupUnregister(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupActivated(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupCleared(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupReset(const FOutcomeEventBase& Outcome);
    void HandleFloorLeaving(const FOutcomeEventBase& Outcome);
    void HandleLevelLoaded(const FOutcomeEventBase& Outcome);

    // ----- Вспомогательные методы для работы с реестром -----
    ASpawnGroupSpawner* FindSpawnerByItemId(const FGuid& ItemId) const;
    ASpawnGroupSpawner* FindSpawnerByGroupId(const FGuid& GroupId) const;

    // ----- Внутренние методы управления (вызываются из обработчиков) -----
    void ActivateSpawnGroupInternal(const FGuid& GroupId);
    void ClearSpawnGroupInternal(const FGuid& GroupId, ESpawnGroupResolutionReason Reason);
    void ResetSpawnGroupInternal(const FGuid& GroupId);

    void UpdateSpawnerStateInCache(ASpawnGroupSpawner* Spawner);

    // ----- Хранилища -----
    UPROPERTY()
    TMap<FGuid, TWeakObjectPtr<ASpawnGroupSpawner>> SpawnerByItemId;      // ItemId -> спавнер

    UPROPERTY()
    TMap<FGuid, FGuid> GroupIdToItemId;                                   // GroupId -> ItemId

    // Сохранённые состояния групп (ключ - этаж, значение - карта группа->состояние)
    TMap<FInteriorFloorKey, TMap<FGuid, FSpawnGroupState>> PersistentGroupStates;

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

    // ----- Хэндлы подписки -----
    FOutcomeHandlerHandle RegisterHandle;
    FOutcomeHandlerHandle UnregisterHandle;
    FOutcomeHandlerHandle ActivateHandle;
    FOutcomeHandlerHandle ClearHandle;
    FOutcomeHandlerHandle ResetHandle;
    FOutcomeHandlerHandle LevelLoadedHandle;
    FOutcomeHandlerHandle FloorLeavingHandle;
    FOutcomeHandlerHandle GhostCapturedHandle;

    // ----- Состояние -----
    bool bIsLoadComplete = true;

    // Текущий ключ этажа (для агрегированных подсчётов)
    FInteriorFloorKey CurrentFloorKey;
};