#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "ActorStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStartedEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionOrGhostEvent, const FOutcomeEventBase&, Outcome);
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
class FPSKITALSREFACTORED_API UActorStateSubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Conditions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> DialogueStartedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionOrGhostCondition;

	// Events
	UPROPERTY(BlueprintAssignable, Category = "ActorStateSubsystem|Events")
	FOnDialogueStartedEvent OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "ActorStateSubsystem|Events")
	FOnMissionOrGhostEvent OnMissionOrGhostWithSpawn;

	// Legacy monitoring broadcast (kept for tools/debug)
	UPROPERTY(BlueprintAssignable, Category = "ActorStateSubsystem|Events")
	FOnActorInteractItemRegistrationEvent OnInteractItemRegistered;

	UPROPERTY(BlueprintAssignable, Category = "ActorStateSubsystem|Events")
	FOnActorInteractItemRegistrationEvent OnInteractItemUnregistered;

	// Subscription APIs
	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeDialogueStarted();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeDialogueStarted();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeMissionOrGhost();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeMissionOrGhost();

	// Registration subscription
	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeRegistration();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeAll();

	// Per-item listener API: реализовано в базовом FInteractiveSubsystemMethods
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

	// Setters for runtime condition assignment
	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SetDialogueStartedCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SetMissionOrGhostCondition(UOutcomeConditionAsset* NewCondition);

private:
	// Handlers
	void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
	void HandleDialogueStarted(const FOutcomeEventBase& Outcome);
	void HandleMissionOrGhost(const FOutcomeEventBase& Outcome);

	// Runtime condition assets to keep alive
	UPROPERTY()
	UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;

	UPROPERTY()
	UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;

	// Handler handles
	FOutcomeHandlerHandle DialogueStartedHandle;
	FOutcomeHandlerHandle MissionOrGhostHandle;
	FOutcomeHandlerHandle RegisteredRegisterHandle;
	FOutcomeHandlerHandle UnregisteredRegisterHandle;

	// Registry of interactive items
	UPROPERTY()
	TMap<FGuid, FActorInteractItemRecord> RegisteredItems;

	// Per-item listeners: only invoked for matching ItemId (локальный контейнер)
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	// Cached EventBus
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

	// Реализация абстрактного доступа для FInteractiveSubsystemMethods
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
	{
		return RegistrationListeners;
	}
};
