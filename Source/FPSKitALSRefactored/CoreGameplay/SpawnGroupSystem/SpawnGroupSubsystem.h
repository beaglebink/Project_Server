#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "GhostClearedOutcome.h"
#include "SpawnGroupSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnGhostClearedEvent, const FGhostClearedOutcome&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API USpawnGroupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnSpawnGhostClearedEvent OnGhostCleared;

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);
};
