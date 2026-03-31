#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "TerminalSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalTaskEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UTerminalSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> TerminalTaskCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnTerminalTaskEvent OnTerminalTaskCompleted;

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void SubscribeTerminalTask();

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void UnsubscribeTerminalTask();

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	void SetTerminalTaskCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "TerminalSubsystem|Handlers")
	bool IsTerminalTaskSubscribed() const { return TerminalTaskHandle.IsValid(); }

private:
	void HandleTerminalTask(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle TerminalTaskHandle;
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
};
