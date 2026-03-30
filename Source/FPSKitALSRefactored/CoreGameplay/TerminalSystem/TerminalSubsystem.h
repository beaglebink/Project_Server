#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "TerminalTaskOutcome.h"
#include "TerminalSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalTaskEvent, const FTerminalTaskOutcome&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UTerminalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> TerminalTaskCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnTerminalTaskEvent OnTerminalTaskCompleted;

private:
	void HandleTerminalTask(const FOutcomeEventBase& Outcome);
};
