#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../SpawnGroupSystem/GhostClearedOutcome.h"
#include "MissionProgressOutcome.h"
#include "MissionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGhostClearedEvent,    const FGhostClearedOutcome&,    Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionProgressEvent, const FMissionProgressOutcome&,  Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Condition assets assigned in Editor Details panel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionProgressCondition;

	// Blueprint delegates - fired after C++ logic
	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionProgressEvent OnMissionProgress;

	// ===== HANDLER MANAGEMENT =====

	// Subscribe handler - can be called multiple times to re-subscribe
	// (Подписать обработчик - можно вызывать повторно для переподписки)
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SubscribeMissionProgress();

	// Unsubscribe specific handler - useful for one-shot or toggleable handlers
	// (Отписать конкретный обработчик - для одноразовых или отключаемых обработчиков)
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeMissionProgress();

	// Unsubscribe all handlers of this subsystem
	// (Отписать все обработчики этой подсистемы)
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeAll();

	// Check subscription state (Проверить состояние подписки)
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	bool IsGhostClearedSubscribed() const { return GhostClearedHandle.IsValid(); }

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	bool IsMissionProgressSubscribed() const { return MissionProgressHandle.IsValid(); }

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);
	void HandleMissionProgress(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle GhostClearedHandle;
	FOutcomeHandlerHandle MissionProgressHandle;
};
