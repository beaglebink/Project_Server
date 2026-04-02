#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "InteriorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorGhostClearedEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorItemAcquiredEvent,  const FOutcomeEventBase&, Outcome);

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
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Ghost cleared condition to control subsystem state
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InteriorSubsystem|Conditions")
	UOutcomeConditionAsset* GhostClearedCondition = nullptr;

	// Item acquired condition to control subsystem state
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "InteriorSubsystem|Conditions")
	UOutcomeConditionAsset* ItemAcquiredCondition = nullptr;

	// ===== Ghost cleared handling =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|GhostCleared")
	void SubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|GhostCleared")
	void UnsubscribeGhostCleared();

	// ===== Item acquired handling =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|ItemAcquired")
	void SubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|ItemAcquired")
	void UnsubscribeItemAcquired();

	// ===== Interaction registration =====
	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void SubscribeInteractionRegistration();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Interaction")
	void UnsubscribeInteractionRegistration();

	// Events
	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "InteriorSubsystem|Events")
	FOnInteriorItemAcquiredEvent OnItemAcquired;

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

	// Handles for logical event listeners
	FOutcomeHandlerHandle GhostClearedHandle;
	FOutcomeHandlerHandle ItemAcquiredHandle;

	// Registry of interactive items
	TMap<FGuid, FInteractItemRecord> RegisteredItems;

	// Cached EventBus subsystem for outcome publishing
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

	// Helpers to subscribe/unsubscribe groups
	void SubscribeAll();
	void UnsubscribeAll();

	// Condition evaluation for incoming outcomes
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);
	void HandleItemAcquired(const FOutcomeEventBase& Outcome);

	// Optional debug helper
	void EvaluateConditions();
};
