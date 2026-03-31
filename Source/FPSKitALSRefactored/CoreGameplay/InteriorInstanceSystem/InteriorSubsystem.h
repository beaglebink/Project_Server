#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "InteriorSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorGhostClearedEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteriorItemAcquiredEvent,  const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UInteriorSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> ItemAcquiredCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInteriorGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnInteriorItemAcquiredEvent OnItemAcquired;

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void SubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void SubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void UnsubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void UnsubscribeItemAcquired();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	void SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	bool IsGhostClearedSubscribed() const { return GhostClearedHandle.IsValid(); }

	UFUNCTION(BlueprintCallable, Category = "InteriorSubsystem|Handlers")
	bool IsItemAcquiredSubscribed() const { return ItemAcquiredHandle.IsValid(); }

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);
	void HandleItemAcquired(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle GhostClearedHandle;
	FOutcomeHandlerHandle ItemAcquiredHandle;
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
};
