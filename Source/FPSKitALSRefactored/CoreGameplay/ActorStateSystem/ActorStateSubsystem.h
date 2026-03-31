#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ActorStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStartedEvent,         const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionOrGhostWithSpawnEvent, const FOutcomeEventBase&, Outcome);

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

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeDialogueStarted();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void SubscribeMissionOrGhost();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeDialogueStarted();

	UFUNCTION(BlueprintCallable, Category = "ActorStateSubsystem|Handlers")
	void UnsubscribeMissionOrGhost();

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

private:
	void HandleDialogueStarted(const FOutcomeEventBase& Outcome);
	void HandleMissionOrGhost(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle DialogueStartedHandle;
	FOutcomeHandlerHandle MissionOrGhostHandle;
};
