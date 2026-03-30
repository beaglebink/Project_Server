#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ItemAcquiredOutcome.h"
#include "InventorySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemAcquiredEvent, const FItemAcquiredOutcome&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UInventorySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> ItemAcquiredCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnItemAcquiredEvent OnItemAcquired;

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void SubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "InventorySubsystem|Handlers")
	bool IsItemAcquiredSubscribed() const { return ItemAcquiredHandle.IsValid(); }

private:
	void HandleItemAcquired(const FOutcomeEventBase& Outcome);
	FOutcomeHandlerHandle ItemAcquiredHandle;
};
