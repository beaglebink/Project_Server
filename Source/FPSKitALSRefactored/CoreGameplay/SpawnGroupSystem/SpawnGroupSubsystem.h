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

    // ----- ISaveableSubsystem -----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("SpawnGroupSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

private:
    // ----- Подписка на события EventBus -----
    void SubscribeEvents();
    void UnsubscribeEvents();

    // Обработчики событий
    void HandleSpawnGroupRegister(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupUnregister(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupActivated(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupCleared(const FOutcomeEventBase& Outcome);
    void HandleSpawnGroupReset(const FOutcomeEventBase& Outcome);

    // Вспомогательные методы для работы с реестром
    ASpawnGroupSpawner* FindSpawnerByItemId(const FGuid& ItemId) const;
    ASpawnGroupSpawner* FindSpawnerByGroupId(const FSpawnGroupId& GroupId) const;

    // Внутренние методы управления (вызываются из обработчиков)
    void ActivateSpawnGroupInternal(const FSpawnGroupId& GroupId);
    void ClearSpawnGroupInternal(const FSpawnGroupId& GroupId, ESpawnGroupResolutionReason Reason);
    void ResetSpawnGroupInternal(const FSpawnGroupId& GroupId);

    // ----- Хранилища -----
    TMap<FGuid, TWeakObjectPtr<ASpawnGroupSpawner>> SpawnerByItemId;      // ItemId -> спавнер
    TMap<FSpawnGroupId, FGuid> GroupIdToItemId;                           // GroupId -> ItemId
    TMap<FSpawnGroupId, FSpawnGroupState> ActiveGroupStates;              // временные состояния
    TMap<FInteriorFloorKey, TMap<FSpawnGroupId, FSpawnGroupState>> PersistentGroupStates; // сохранённые

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

    // Хэндлы подписки
    FOutcomeHandlerHandle RegisterHandle;
    FOutcomeHandlerHandle UnregisterHandle;
    FOutcomeHandlerHandle ActivateHandle;
    FOutcomeHandlerHandle ClearHandle;
    FOutcomeHandlerHandle ResetHandle;

    bool bIsLoadComplete = true;
};