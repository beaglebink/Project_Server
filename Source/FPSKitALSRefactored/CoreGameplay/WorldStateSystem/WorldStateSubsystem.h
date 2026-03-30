#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "MissionCompletedOutcome.h"
#include "WorldStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionCompletedEvent, const FMissionCompletedOutcome&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UWorldStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionCompletedCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionCompletedEvent OnMissionCompleted;

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void SubscribeMissionCompleted();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void UnsubscribeMissionCompleted();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	bool IsMissionCompletedSubscribed() const { return MissionCompletedHandle.IsValid(); }

private:
	void HandleMissionCompleted(const FOutcomeEventBase& Outcome);
	FOutcomeHandlerHandle MissionCompletedHandle;
};
