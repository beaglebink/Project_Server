#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "ActorStateOutcome.h"
#include "ActorStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStartedEvent,      const FActorStateOutcome&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionOrGhostWithSpawnEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UActorStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> DialogueStartedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionOrGhostCondition;

	// Typed delegate for dialogue - Blueprint gets FActorStateOutcome directly
	// (“ипизированный делегат дл€ диалога - Blueprint получает FActorStateOutcome напр€мую)
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnDialogueStartedEvent OnDialogueStarted;

	// Base type delegate - event can be MissionCompleted or GhostCleared
	// (ƒелегат базового типа - событие может быть MissionCompleted или GhostCleared)
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionOrGhostWithSpawnEvent OnMissionOrGhostWithSpawn;

private:
	void HandleDialogueStarted(const FOutcomeEventBase& Outcome);
	void HandleMissionOrGhost(const FOutcomeEventBase& Outcome);
};
