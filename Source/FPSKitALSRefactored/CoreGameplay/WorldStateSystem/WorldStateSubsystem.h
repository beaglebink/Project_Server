#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "WorldStateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChangingLocationAvailabilityEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UWorldStateSubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Condition set in Editor or Blueprint (можно присвоить в BP)
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
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
	// Blueprint must call this method after assigning Condition (for example, in BeginPlay)
	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	void SetChangingLocationAvailabilityCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "WorldStateSubsystem|Handlers")
	bool IsChangingLocationAvailabilitySubscribed() const { return ChangingLocationAvailabilityHandle.IsValid(); }

	// Per-item listener API provided by FInteractiveSubsystemMethods (if needed)
	void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
	void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

private:
	void HandleChangingLocationAvailability(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle ChangingLocationAvailabilityHandle;

	// Per-item listeners (локальный контейнер для этой подсистемы)
	TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

	// Реализация абстрактного доступа для FInteractiveSubsystemMethods
	virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
	{
		return RegistrationListeners;
	}
};
