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
	TObjectPtr<UOutcomeConditionAsset> ChangingLocationAvailabilityCondition;

	// Condition / handle for receiving WorldStateRecord commands via EventBus
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> WorldStateRecordCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnChangingLocationAvailabilityEvent OnChangingLocationAvailability;

	// ===== HANDLER MANAGEMENT =====
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void SubscribeChangingLocationAvailability();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void UnsubscribeChangingLocationAvailability();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void UnsubscribeAll();

	// Blueprint должен вызвать этот метод после присвоения Condition (например, в BeginPlay)
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void SetChangingLocationAvailabilityCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	bool IsChangingLocationAvailabilitySubscribed() const { return ChangingLocationAvailabilityHandle.IsValid(); }

	// Per-item listener API
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

	// ===== WORLD STATE RECORDS API =====

	// Добавить или перезаписать запись об изменении мира.
	// Если запись с таким ItemId+ChangeKey уже есть — перезаписывается.
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
	void SetWorldStateRecord(const FWorldStateRecord& Record);

	// Проверить наличие записи
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "WorldStateSubsystem|State")
	bool HasWorldStateRecord(const FGuid& ItemId, FName ChangeKey) const;

	// Получить запись (возвращает false если не найдена)
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
	bool GetWorldStateRecord(const FGuid& ItemId, FName ChangeKey, FWorldStateRecord& OutRecord) const;

	// Удалить запись
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
	void RemoveWorldStateRecord(const FGuid& ItemId, FName ChangeKey);

	// Получить все записи для конкретного объекта
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
	TArray<FWorldStateRecord> GetRecordsForItem(const FGuid& ItemId) const;

	// Получить все записи указанной категории
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
	TArray<FWorldStateRecord> GetRecordsByCategory(EWorldStateChangeCategory Category) const;

	// Применить все записи к миру после загрузки
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|State")
	void ApplyRecordsToWorld();

	// ===== ISaveableSubsystem =====
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
	virtual FString GetSaveSubsystemName() const override { return TEXT("WorldStateSubsystem"); }

private:
	void HandleChangingLocationAvailability(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle ChangingLocationAvailabilityHandle;
	FOutcomeHandlerHandle WorldStateRecordHandle;

	// Handler for incoming WorldStateRecord payloads
	void HandleSetWorldStateRecord(const FOutcomeEventBase& Outcome);

	// Per-item listeners
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
	{
		return RegistrationListeners;
	}

	// ─── Хранилище постоянных изменений мира ─────────────────────────────────
	// Ключ внешний: ItemId (стабильный GUID объекта из UFloorAssignmentComponent)
	// Ключ внутренний: ChangeKey (конкретное свойство/флаг)
	TMap<FGuid, TMap<FName, FWorldStateRecord>> WorldStateRecords;

	// Применить одну запись к актору
	void ApplyRecordToActor(AActor* Actor, const FWorldStateRecord& Record) const;
};

