#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../SpawnGroupSystem/GhostClearedOutcome.h"
#include "MissionProgressOutcome.h"
#include "MissionSubsystem.generated.h"

// Typed delegates for Blueprint - Blueprint receives correct struct directly
// (Типизированные делегаты для Blueprint - Blueprint получает правильную структуру напрямую)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGhostClearedEvent,   const FGhostClearedOutcome&,   Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionProgressEvent, const FMissionProgressOutcome&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Condition assets - assigned in Editor Details panel
	// (Asset условий - назначаются в Details панели редактора)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionProgressCondition;

	// Typed Blueprint delegates - no casting needed in Blueprint
	// (Типизированные Blueprint делегаты - каст в Blueprint не нужен)
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionProgressEvent OnMissionProgress;

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);
	void HandleMissionProgress(const FOutcomeEventBase& Outcome);
};
