#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "ActorStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStartedEvent,         const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionOrGhostWithSpawnEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorInteractItemRegistrationEvent, UInteractItemRegistrationPayload*, Payload);

class UInteractiveItemComponent;

// Record stored for each registered interactive item (Actor)
USTRUCT()
struct FActorInteractItemRecord
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
class FPSKITALSREFACTORED_API UActorStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> DialogueStartedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionOrGhostCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnDialogueStartedEvent OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionOrGhostWithSpawnEvent OnMissionOrGhostWithSpawn;

	// Legacy monitoring broadcast (kept for tools/debug)
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnActorInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnActorInteractItemRegistrationEvent OnInteractItemUnregistered;

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeDialogueStarted();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeMissionOrGhost();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeDialogueStarted();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeMissionOrGhost();

	// Subscribe/unsubscribe interaction registration (Actor-specific)
	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SetDialogueStartedCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SetMissionOrGhostCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	bool IsDialogueStartedSubscribed() const { return DialogueStartedHandle.IsValid(); }

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	bool IsMissionOrGhostSubscribed() const { return MissionOrGhostHandle.IsValid(); }

	// Per-item listener API: only the matching component gets notified
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

private:
	void HandleDialogueStarted(const FOutcomeEventBase& Outcome);
	void HandleMissionOrGhost(const FOutcomeEventBase& Outcome);
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle DialogueStartedHandle;
	FOutcomeHandlerHandle MissionOrGhostHandle;

	// Handles for interaction registration
	FOutcomeHandlerHandle RegisteredRegisterHandle;
	FOutcomeHandlerHandle UnregisteredRegisterHandle;

	// Runtime condition assets
	UPROPERTY()
	UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;

	// Registry
	UPROPERTY()
	TMap<FGuid, FActorInteractItemRecord> RegisteredItems;

	// Per-item listeners
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	// Cached EventBus
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
};
