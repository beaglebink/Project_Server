#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h" // <-- обязательно: определение миксина должно быть видимо до generated.h

#include "InteriorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

class UInteractiveItemComponent;

// Record stored for each registered interactive item
struct FInteractItemRecord
{
	FGuid ItemId;
	EInteractiveSubsystem SubsystemType;
	float InteractionRange;
	FText DefaultTooltip;
	TWeakObjectPtr<AActor> OwnerActor;
};

UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UWorldSubsystem, public FInteractiveSubsystemMethods
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// ===== Interaction registration =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeInteractionRegistration();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeInteractionRegistration();

	// ===== Interact command handling (EventBus) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeInteractCommand();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeInteractCommand();

	// ===== SetEnabled command (вкл/выкл интерактивного компонента) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeSetEnabled();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeSetEnabled();

	// ===== SetRange command (установка диапазона взаимодействия) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeSetRange();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeSetRange();

	// ===== SetTooltip command (установка подсказки взаимодействия) =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeSetTooltip();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeSetTooltip();

	// Per-item listener API (interior): only the matching component gets notified
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

	// Legacy monitoring broadcast (kept for tools/debug)
	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorInteractItemRegistrationEvent OnInteractItemUnregistered;

private:
	// Handler for spawn/despawn interactive objects
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);

	// Runtime condition assets to keep alive
	UPROPERTY()
	UOutcomeConditionAsset* SpawnConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* DespawnConditionAsset = nullptr;

	// Handles for interaction registration
	FOutcomeHandlerHandle SpawnRegisterHandle;
	FOutcomeHandlerHandle DespawnRegisterHandle;

	// Registry of interactive items
	TMap<FGuid, FInteractItemRecord> RegisteredItems;

	// Per-item listeners: only invoked for matching ItemId
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	// Cached EventBus subsystem for outcome publishing
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

	// Helpers to subscribe/unsubscribe groups
	void SubscribeAll();
	void UnsubscribeAll();

	// Optional debug helper
	void EvaluateConditions();

private:
	// Interact command handler and runtime condition/handle
	void HandleInteractCommand(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;

	FOutcomeHandlerHandle InteractCommandHandle;

	// SetEnabled command handler and runtime condition/handle
	void HandleSetEnabled(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* SetEnabledConditionAsset = nullptr;

	FOutcomeHandlerHandle SetEnabledHandle;

	// SetRange command handler and runtime condition/handle
	void HandleSetRange(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;

	FOutcomeHandlerHandle SetRangeHandle;

	// SetTooltip command handler and runtime condition/handle
	void HandleSetTooltip(const FOutcomeEventBase& Outcome);

	UPROPERTY()
	UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;

	FOutcomeHandlerHandle SetTooltipHandle;

	// Реализация абстрактного доступа для FInteractiveSubsystemMethods
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
	{
		return RegistrationListeners;
	}
};
