#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include <SpawnGroupAsset.h>
#include "SpawnGroupSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpawnGhostClearedEvent, const FOutcomeEventBase&, Outcome);

UCLASS()
class FPSKITALSREFACTORED_API USpawnGroupSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

    // Subsystem listens for GhostCleared outcomes and broadcasts them via Blueprint events.
    // (Подсистема слушает исходы GhostCleared и ретранслирует их через Blueprint-события.)
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

	// Применить группу (вызывается из миссии или автоматически при входе на этаж)
	UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
	void ActivateSpawnGroup(const FSpawnGroupId& GroupId, const FGuid& InteriorSetId, const FGuid& FloorId, FName MissionContext);

	// Отметить элемент группы как очищенный (уничтожен/захвачен)
	UFUNCTION(BlueprintCallable, Category = "SpawnGroup")
	void ReportGhostCleared(const FSpawnGroupId& GroupId, const FGuid& ActorId, ESpawnGroupResolutionReason Reason);

	// Получить политику для группы с учётом текущей активной миссии (через канал SpawnGroups)
	EChannelPolicy GetEffectivePolicyForGroup(const FSpawnGroupId& GroupId, EMissionEndReason EndReason = EMissionEndReason::None) const;

private:
	void HandleGhostCleared(const FOutcomeEventBase& Outcome);

	FOutcomeHandlerHandle GhostClearedHandle;
	TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;

	void HandleSpawnGroupActivationCommand(const FOutcomeEventBase& Outcome);
	void HandleSpawnGroupClearCommand(const FOutcomeEventBase& Outcome);
	void HandleSpawnGroupResetCommand(const FOutcomeEventBase& Outcome);
	void HandleFloorTransition(const FOutcomeEventBase& Outcome); // для сохранения/сброса при уходе

	// Кеш активных групп для текущего этажа
	TMap<FSpawnGroupId, FSpawnGroupState> ActiveGroups;
};
