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
    TObjectPtr<UOutcomeConditionAsset> ChangingLocationAvailabilityCondition;

    // Условие для команды установки записи
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> WorldStateRecordCondition;

    // Условие для команды удаления записи
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
    TObjectPtr<UOutcomeConditionAsset> WorldStateRecordRemoveCondition;

    // ===== СОБЫТИЕ ДЛЯ BLUEPRINT =====

    UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
    FOnChangingLocationAvailabilityEvent OnChangingLocationAvailability;

    // ===== УПРАВЛЕНИЕ ПОДПИСКАМИ (публичные) =====

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    void SubscribeChangingLocationAvailability();

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    void UnsubscribeChangingLocationAvailability();

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    void UnsubscribeAll();

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    void SetChangingLocationAvailabilityCondition(UOutcomeConditionAsset* NewCondition);

    UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
    bool IsChangingLocationAvailabilitySubscribed() const { return ChangingLocationAvailabilityHandle.IsValid(); }

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
    // ---- ОБРАБОТЧИКИ СОБЫТИЙ (приватные) ----

    void HandleChangingLocationAvailability(const FOutcomeEventBase& Outcome);
    void HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome);
    void HandleRemoveWorldStateRecord(const FOutcomeEventBase& Outcome);   // <-- объявлен

    // ---- ХЕНДЛЫ ПОДПИСОК ----

    FOutcomeHandlerHandle ChangingLocationAvailabilityHandle;
    FOutcomeHandlerHandle WorldStateRecordHandle;
    FOutcomeHandlerHandle WorldStateRecordRemoveHandle;

    // ---- ПРИВАТНЫЕ МЕТОДЫ ИЗМЕНЕНИЯ СОСТОЯНИЯ (НЕ ДОСТУПНЫ ИЗ BLUEPRINT) ----

    void SetWorldStateRecord(const FWorldStateRecord& Record);
    void RemoveWorldStateRecord(const FGuid& ItemId, FName ChangeKey);
    void ApplyRecordsToWorld();
    void ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const;

    // ---- СЛУШАТЕЛИ PER-ITEM (унаследовано от FInteractiveSubsystemMethods) ----

    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }

    // ---- ХРАНИЛИЩЕ ЗАПИСЕЙ ----

    TMap<FGuid, TMap<FName, FWorldStateRecord>> WorldStateRecords;

    bool IsLoadComplete = true;
};