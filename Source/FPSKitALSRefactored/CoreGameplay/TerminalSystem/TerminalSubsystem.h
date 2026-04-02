#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "TerminalSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalTaskEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

class UInteractiveItemComponent; // forward-declare

// Record stored for each registered interactive item (Terminal)
USTRUCT()
struct FTerminalInteractItemRecord
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
class FPSKITALSREFACTORED_API UTerminalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	// Condition used to filter terminal task events (set in editor or runtime)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> TerminalTaskCondition;

	// Broadcast when a matching terminal task outcome is handled
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnTerminalTaskEvent OnTerminalTaskCompleted;

	// Broadcast when an interactive item is registered/unregistered
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInteractItemRegistrationEvent OnInteractItemUnregistered;

	// Subscribe/unsubscribe task handling
	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void SubscribeTerminalTask();

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void UnsubscribeTerminalTask();

	// Subscribe/unsubscribe registration (InteractRegistered / InteractUnregistered) listeners
	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void SubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void UnsubscribeRegistration();

	// Unsubscribe everything
	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void UnsubscribeAll();

	// Set task condition at runtime (compiles condition and (re)subscribes)
	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void SetTerminalTaskCondition(UOutcomeConditionAsset* NewCondition);

	// Per-item listener API: components register a listener for their ItemId.
	// The subsystem will call the listener only for that specific item (prevents notifying all components).
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

private:
	// Handler for terminal task outcomes (bound when TerminalTaskCondition is set)
	void HandleTerminalTask(const FOutcomeEventBase& Outcome);

	// Handler for registration/unregistration of interactive items for terminal subsystem
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);

	// Handler descriptor for task condition
	FOutcomeHandlerHandle TerminalTaskHandle;

	// Handles for registration listeners
	FOutcomeHandlerHandle RegisteredRegisterHandle;
	FOutcomeHandlerHandle UnregisteredRegisterHandle;

	// Runtime condition assets used to listen for registration/unregistration (kept as UPROPERTY so GC is safe)
	UPROPERTY()
	UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;

	// Registry of interactive items (key = ItemId)
	UPROPERTY()
	TMap<FGuid, FTerminalInteractItemRecord> RegisteredItems;

	// Per-item listeners (only invoked for matching ItemId)
	// Not a UPROPERTY because it holds weak refs to components
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	// Cached EventBus subsystem pointer
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
};
