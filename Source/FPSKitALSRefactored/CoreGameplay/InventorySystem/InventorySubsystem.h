#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "InventorySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAcquiredEvent,                   const FOutcomeEventBase&,        Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

class UInteractiveItemComponent;

// Record stored for each registered interactive item (Inventory/Object)
USTRUCT()
struct FInventoryInteractItemRecord
{
	GENERATED_BODY()
	UPROPERTY()
	FGuid ItemId;
	UPROPERTY()
	TObjectPtr<AActor> OwnerActor = nullptr;
	UPROPERTY()
	float InteractionRange = 0.f;
};

UCLASS()
class FPSKITALSREFACTORED_API UInventorySubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> ItemAcquiredCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnItemAcquiredEvent OnItemAcquired;

	// Legacy monitoring broadcast (kept for tools/debug)
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInventoryInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInventoryInteractItemRegistrationEvent OnInteractItemUnregistered;

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	bool IsItemAcquiredSubscribed() const { return ItemAcquiredHandle.IsValid(); }

	// Per-item listener API: only the matching component gets notified
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeInteractCommand();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeInteractCommand();

	// ----- SetEnabled (вкл/выкл интеракции) -----
	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeSetEnabled();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeSetEnabled();

	// ----- SetRange (изменение радиуса) -----
	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeSetRange();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeSetRange();

	// ----- SetTooltip (изменение текста подсказки) -----
	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeSetTooltip();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeSetTooltip();

private:
	void HandleItemAcquired(const FOutcomeEventBase& Outcome);
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
	void HandleInteractCommand(const FOutcomeEventBase& Outcome);

	// Handlers for new commands
	void HandleSetEnabled(const FOutcomeEventBase& Outcome);
	void HandleSetRange(const FOutcomeEventBase& Outcome);
	void HandleSetTooltip(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle ItemAcquiredHandle;
	FOutcomeHandlerHandle RegisteredRegisterHandle;
	FOutcomeHandlerHandle UnregisteredRegisterHandle;
	FOutcomeHandlerHandle InteractCommandHandle;

	// For SetEnabled / SetRange
	FOutcomeHandlerHandle SetEnabledHandle;
	FOutcomeHandlerHandle SetRangeHandle;
	FOutcomeHandlerHandle SetTooltipHandle;

	UPROPERTY()
	UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;

	// Condition assets for SetEnabled / SetRange
	UPROPERTY()
	UOutcomeConditionAsset* SetEnabledConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;

	UPROPERTY()
	TMap<FGuid, FInventoryInteractItemRecord> RegisteredItems;

	// Per-item listeners
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

private:
	// Реализация абстрактного доступа для FInteractiveSubsystemMethods
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
	{
		return RegistrationListeners;
	}
};
