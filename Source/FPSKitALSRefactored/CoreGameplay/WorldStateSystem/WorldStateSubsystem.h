#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "WorldStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChangingLocationAvailabilityEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UWorldStateSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Condition set in Editor or Blueprint (можно присвоить в BP)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> ChangingLocationAvailabilityCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnChangingLocationAvailabilityEvent OnChangingLocationAvailability;

	// ===== HANDLER MANAGEMENT =====
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void SubscribeChangingLocationAvailability();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void UnsubscribeChangingLocationAvailability();

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void UnsubscribeAll();

	// Blueprint должен вызвать этот метод после присвоения Condition (например, в BeginPlay)
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void SetChangingLocationAvailabilityCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	bool IsChangingLocationAvailabilitySubscribed() const { return ChangingLocationAvailabilityHandle.IsValid(); }

private:
	void HandleChangingLocationAvailability(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle ChangingLocationAvailabilityHandle;
};
