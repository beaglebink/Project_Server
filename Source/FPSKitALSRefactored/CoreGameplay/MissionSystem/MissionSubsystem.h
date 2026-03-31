#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "MissionSubsystem.generated.h"

// All handlers receive FOutcomeEventBase - cast Payload to concrete type inside handler
// (Все обработчики получают FOutcomeEventBase - кастуй Payload к конкретному типу внутри)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGhostClearedEvent,    const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionProgressEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API UMissionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Condition assets assigned in Editor Details panel or at runtime via Set*
	// (Ассеты условий можно присвоить в редакторе или в рантайме через Set*)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionProgressCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionProgressEvent OnMissionProgress;

	// ===== HANDLER MANAGEMENT =====
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SubscribeMissionProgress();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeGhostCleared();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeMissionProgress();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeAll();

	// Set condition at runtime and (re)subscribe automatically
	// (Присвоить условие в рантайме и автоматически (пере)подписаться)
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SetGhostClearedCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SetMissionProgressCondition(UOutcomeConditionAsset* NewCondition);

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
