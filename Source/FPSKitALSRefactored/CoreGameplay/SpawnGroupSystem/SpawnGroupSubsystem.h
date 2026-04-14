#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "SpawnGroupSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnGhostClearedEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API USpawnGroupSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnSpawnGhostClearedEvent OnGhostCleared;

	UFUNCTION(BlueprintCallable, Category = "SpawnGroupSubsystem|Handlers")
	void SubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "SpawnGroupSubsystem|Handlers")
	void UnsubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "SpawnGroupSubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "SpawnGroupSubsystem|Handlers")
	void SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "SpawnGroupSubsystem|Handlers")
	bool IsGhostClearedSubscribed() const { return GhostClearedHandle.IsValid(); }

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle GhostClearedHandle;
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
};
