#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "MissionController.h"
#include "MissionAsset.h"
#include "MissionEnvelopeTypes.h"
#include "MissionEnvelopePayload.h"
#include "ISaveableSubsystem.h"
#include "FloorPopulationTypes.h"
#include "../LocationSystem/FloorAsset.h"
#include "MissionSubsystem.generated.h"

// All handlers receive FOutcomeEventBase - cast Payload to concrete type inside handler
// (Все обработчики получают FOutcomeEventBase — внутри приведите Payload к конкретному типу)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGhostClearedEvent,    const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionProgressEvent, const FOutcomeEventBase&, Outcome);

// ─── FActiveMissionEntry ──────────────────────────────────────────────────────
// Запись об активной миссии в MissionSubsystem.
// MissionId = FName имени ассета (стабильный, читаемый).
struct FActiveMissionEntry
{
	FName MissionId;
	TObjectPtr<UMissionController> Controller;
};

// ─── FEnvelopeConflictInfo ────────────────────────────────────────────────────
// Результат проверки конфликта envelope'ов по scope + channel.
struct FEnvelopeConflictInfo
{
	// true — конфликт есть, активный envelope побеждает по приоритету
	bool bHasConflict = false;
	// Имя миссии-победителя (меньший Priority)
	FName WinnerMissionId;
	// Имя миссии-проигравшей (больший Priority — молча откладывает)
	FName LoserMissionId;
};

UCLASS()
class FPSKITALSREFACTORED_API UMissionSubsystem : public UGameInstanceSubsystem, public ISaveableSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ===== MISSION LIFECYCLE =====

	// Создать и зарегистрировать миссию по DataAsset.
	// Возвращает Controller — Blueprint-логика должна его сохранить.
	// Envelope проверяется на конфликты с уже активными миссиями.
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Missions")
	UMissionController* CreateMission(UMissionAsset* MissionAsset);

	// Активировать ранее созданную миссию.
	// Вызывает Controller->Activate() и регистрирует envelope в InteriorSubsystem.
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Missions")
	void ActivateMission(FName MissionId);

	// Завершить миссию с указанием причины.
	// Вызывает Controller->RequestResolve(), затем обрабатывает Envelope release.
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Missions")
	void ResolveMission(FName MissionId, EMissionEndReason Reason);

	// Получить контроллер активной миссии по ID
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionSubsystem|Missions")
	UMissionController* GetMissionController(FName MissionId) const;

	// Проверить есть ли активная миссия с данным ID
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionSubsystem|Missions")
	bool IsMissionActive(FName MissionId) const;

	// Список ID всех активных миссий
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionSubsystem|Missions")
	TArray<FName> GetActiveMissionIds() const;

	// ===== CONFLICT RESOLUTION =====

	// Проверить конфликт нового envelope с активными миссиями по scope + channel.
	// Возвращает список конфликтов (для каждого затронутого канала).
	TArray<FEnvelopeConflictInfo> CheckEnvelopeConflicts(
		FName NewMissionId,
		const FMissionEnvelope& NewEnvelope) const;

	// Найти "победителя" для конкретного канала в конкретной зоне.
	// Возвращает MissionId с наименьшим Priority (или NAME_None если нет владельца).
	FName GetChannelOwner(
		const FInteriorFloorKey& FloorKey,
		EEnvelopeChannel Channel) const;

	// ===== BUILDING EXIT HANDLING =====

	// Вызывается InteriorSubsystem когда игрок покидает здание.
	// MissionSubsystem уведомляет контроллеры чьи Scope включают это здание.
	void NotifyBuildingExited(const FText& BuildingDisplayName);

	// ===== EventBus подписка на envelope-события =====

	// Blueprint может публиковать MissionActivated через EventBus
	// вместо прямого вызова CreateMission/ActivateMission.
	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|EventBus")
	void SubscribeMissionEnvelopeEvents();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|EventBus")
	void UnsubscribeMissionEnvelopeEvents();

	// Ассеты условий для envelope-событий.
	// Назначаются в редакторе или через Blueprint после StartUp.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> MissionActivatedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> MissionResolvedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> BuildingLeavingCondition;

	// ===== ISaveableSubsystem =====
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
	virtual FString GetSaveSubsystemName() const override { return TEXT("MissionSubsystem"); }

	// ===== LEGACY HANDLER MANAGEMENT (сохранено для совместимости) =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionProgressCondition;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnGhostClearedEvent OnGhostCleared;

	UPROPERTY(BlueprintAssignable, Category = "EventBus|Events")
	FOnMissionProgressEvent OnMissionProgress;

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

	// Handles для envelope-событий (MissionActivated, Resolve, BuildingLeaving)
	FOutcomeHandlerHandle EnvelopeActivateHandle;
	FOutcomeHandlerHandle EnvelopeResolveHandle;
	FOutcomeHandlerHandle BuildingLeavingHandle;

	void HandleEnvelopeActivate(const FOutcomeEventBase& Outcome);
	void HandleEnvelopeResolve(const FOutcomeEventBase& Outcome);
	void HandleBuildingLeaving(const FOutcomeEventBase& Outcome);

	// ─── Активные миссии ─────────────────────────────────────────────────────
	// Ключ — FName идентификатор миссии (имя ассета)
	TMap<FName, FActiveMissionEntry> ActiveMissions;

	// ─── Вспомогательные методы ───────────────────────────────────────────────

	// Проверяет что scope нового envelope перекрывается со scope существующего
	bool ScopesOverlap(const FMissionEnvelopeScope& A, const FMissionEnvelopeScope& B) const;

	// Проверяет что два envelope имеют хотя бы один общий канал
	bool ChannelsOverlap(const FMissionEnvelope& A, const FMissionEnvelope& B) const;

	// Применить политику Envelope при уходе из здания
	void ApplyEnvelopeExitPolicy(FName MissionId, const FMissionEnvelope& Envelope);

	// Промоутить изменения канала в WorldStateSubsystem (политика Persist)
	void PromoteChannelToWorldState(FName MissionId, const FMissionEnvelope& Envelope);

	// Helper for EventBus registration (реализация в .cpp)
	void TryRegisterCondition(
		TObjectPtr<UOutcomeConditionAsset>& Condition,
		FOutcomeHandlerHandle& Handle,
		void (UMissionSubsystem::* HandlerMethod)(const FOutcomeEventBase&),
		EOutcomeMission MissionFilter = EOutcomeMission::Default);
};
