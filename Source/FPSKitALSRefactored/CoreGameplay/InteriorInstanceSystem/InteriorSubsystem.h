#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../SpawnGroupSystem/GhostClearedOutcome.h"
#include "../InventorySystem/ItemAcquiredOutcome.h"
#include "InteriorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorGhostClearedEvent, const FGhostClearedOutcome&,  Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorItemAcquiredEvent,  const FItemAcquiredOutcome&,  Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> ItemAcquiredCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInteriorGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInteriorItemAcquiredEvent OnItemAcquired;

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);
	void HandleItemAcquired(const FOutcomeEventBase& Outcome);
};
