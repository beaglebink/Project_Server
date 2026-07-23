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
class USpawnGroupSubsystem : public UGameInstanceSubsystem, public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- Статистика по конкретной группе ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetTotalSpawnedCount(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCount(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCount(const FGuid& ItemId) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByType(const FGuid& ItemId, TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTextTag(const FGuid& ItemId, const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByGameplayTag(const FGuid& ItemId, const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    ESpawnGroupStatus GetGroupStatus(const FGuid& ItemId) const;

    // ---- Статистика по текущему этажу (агрегирует все группы) ----
    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetTotalSpawnedCountForCurrentFloor() const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountForCurrentFloor() const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountForCurrentFloor() const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTypeForCurrentFloor(TSubclassOf<AActor> ActorClass) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByTextTagForCurrentFloor(const TArray<FName>& TextTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetKilledCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const;

    UFUNCTION(BlueprintCallable, Category = "SpawnGroup|Stats")
    int32 GetAliveCountByGameplayTagForCurrentFloor(const TArray<FGameplayTag>& GameplayTags) const;

    // ----- ISaveableSubsystem -----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("SpawnGroupSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

private:
    // ----- Подписка на события EventBus -----
    void SubscribeEvents();
    void UnsubscribeEvents();

    FInteriorFloorKey GetFloorKeyFromSpawner(ASpawnGroupSpawner* Spawner) const;

    // Обработчики событий
    void HandleSpawnGroupRegister(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupUnregister(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupActivated(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupCleared(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupReset(const FOutcomeEventBase& Outcome);
    void HandleFloorLeaving(const FOutcomeEventBase& Outcome);
    void HandleLevelLoaded(const FOutcomeEventBase& Outcome);

    // Вспомогательные методы для работы с реестром
    ASpawnGroupSpawner* FindSpawnerByItemId(const FGuid& ItemId) const;
    ASpawnGroupSpawner* FindSpawnerByGroupId(const FGuid& GroupId) const;

    // Внутренние методы управления (вызываются из обработчиков)
    void ActivateSpawnGroupInternal(const FGuid& GroupId);
    void ClearSpawnGroupInternal(const FGuid& GroupId, ESpawnGroupResolutionReason Reason);
    void ResetSpawnGroupInternal(const FGuid& GroupId);

    void UpdateSpawnerStateInCache(ASpawnGroupSpawner* Spawner);

    // ----- Хранилища -----
    UPROPERTY()
    TMap<FGuid, TWeakObjectPtr<ASpawnGroupSpawner>> SpawnerByItemId;      // ItemId -> спавнер

    UPROPERTY()
    TMap<FGuid, FGuid> GroupIdToItemId;                           // GroupId -> ItemId

    TMap<FInteriorFloorKey, TMap<FGuid, FSpawnGroupState>> PersistentGroupStates; // сохранённые

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

    // Хэндлы подписки
    FOutcomeHandlerHandle RegisterHandle;
    FOutcomeHandlerHandle UnregisterHandle;
    FOutcomeHandlerHandle ActivateHandle;
    FOutcomeHandlerHandle ClearHandle;
    FOutcomeHandlerHandle ResetHandle;
    FOutcomeHandlerHandle LevelLoadedHandle;
    FOutcomeHandlerHandle FloorLeavingHandle;

    bool bIsLoadComplete = true;

    FInteriorFloorKey CurrentFloorKey;
};