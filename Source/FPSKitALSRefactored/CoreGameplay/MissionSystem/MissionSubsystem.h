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
#include "CheckRequestPayload.h"
#include "MissionSubsystem.generated.h"


// ─── FActiveMissionEntry ──────────────────────────────────────────────────────
// Запись об активной миссии в MissionSubsystem.
// MissionId = FName имени ассета (стабильный, читаемый).
USTRUCT()
struct FActiveMissionEntry
{
	GENERATED_BODY()

	// Идентификатор миссии (имя ассета)
	UPROPERTY()
	FName MissionId;

	// текущий шаг миссии (индекс в массиве шагов ассета, 0-based)
	UPROPERTY()
	int32 MissionStep = 0;

	// Контроллер миссии — помечен UPROPERTY чтобы GC учитывал ссылку во время загрузок/SeamlessTravel
	UPROPERTY()
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
	// Имя миссии-проигравшей (большой Priority — молча откладывает)
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
	void ActivateMission(FName MissionId, bool IsUpdateSnapshots = true);

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

	// История завершённых миссий (имена миссий)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionSubsystem|History")
	bool IsMissionCompleted(FName MissionId) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionSubsystem|History")
	TArray<FName> GetCompletedMissionIds() const;

	bool IsMissionConflict(FName MissionName, FMissionEnvelope NewMissionEnvelope, FName& ConflictedMissionName);

	// Ассеты условий для envelope-событий.
	// Назначаются в редакторе или через Blueprint после StartUp.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> MissionActivatedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> MissionResolvedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> BuildingLeavingCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope")
	TObjectPtr<UOutcomeConditionAsset> UpdateActiveMissionIdCondition;

	// ===== ISaveableSubsystem =====
	virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
	virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
	virtual FString GetSaveSubsystemName() const override { return TEXT("MissionSubsystem"); }
	virtual bool GetIsLoadComplete() const override { return IsLoadComplete; }

	// ===== LEGACY HANDLER MANAGEMENT (сохранено для совместимости) =====

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> GhostClearedCondition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions")
	TObjectPtr<UOutcomeConditionAsset> MissionProgressCondition;

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SubscribeMissionProgress();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeMissionProgress();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void UnsubscribeAll();

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	void SetMissionProgressCondition(UOutcomeConditionAsset* NewCondition);

	UFUNCTION(BlueprintCallable, Category = "MissionSubsystem|Handlers")
	bool IsMissionProgressSubscribed() const { return MissionProgressHandle.IsValid(); }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MissionSubsystem|Query")
	int32 GetMissionStep(FName MissionId) const;

	void SetActiveMissionId(FName& MissionId) { ActiveMissionId = MissionId; }
	FName GetActiveMissionId() const { return ActiveMissionId; }

	void OnCheckRequest(const FOutcomeEventBase& Event);
	void SubscribeRequests();
	void UnsubscribeRequests();

private:
	// Helper to register runtime condition -> handler (реализовано в .cpp)
	void TryRegisterCondition(
		TObjectPtr<UOutcomeConditionAsset>& Condition,
		FOutcomeHandlerHandle& Handle,
		void (UMissionSubsystem::* HandlerMethod)(const FOutcomeEventBase&),
		EOutcomeMission MissionFilter = EOutcomeMission::Default);

	void HandleMissionProgress(const FOutcomeEventBase& Outcome);

	//FOutcomeHandlerHandle GhostClearedHandle;
	FOutcomeHandlerHandle MissionProgressHandle;

	// Handles для envelope-событий (MissionActivated, Resolve, BuildingLeaving)
	FOutcomeHandlerHandle EnvelopeActivateHandle;
	FOutcomeHandlerHandle EnvelopeResolveHandle;
	FOutcomeHandlerHandle BuildingLeavingHandle;
	// Handle для подтверждения релиза миссии (публикуется InteriorSubsystem)
	FOutcomeHandlerHandle MissionReleasedHandle;
	FOutcomeHandlerHandle UpdateActiveMissionIdHandle;
	FOutcomeHandlerHandle CheckRequestHandle;
	UPROPERTY()
	TObjectPtr<UOutcomeConditionAsset> MissionReleasedCondition;

	bool IsMissionActiveOnCurrentLevel(FName MissionId) const;

	void HandleEnvelopeActivate(const FOutcomeEventBase& Outcome);
	void HandleUpdateActiveMissionId(const FOutcomeEventBase& Outcome);
	void HandleEnvelopeResolve(const FOutcomeEventBase& Outcome);
	void HandleBuildingLeaving(const FOutcomeEventBase& Outcome);
	void HandleMissionReleased(const FOutcomeEventBase& Outcome);

	// ─── Активные миссии ─────────────────────────────────────────────────────
	// Ключ — FName идентификатор миссии (имя ассета)
	// Контейнер помечен как UPROPERTY чтобы GC видел внутренние UPROPERTY поля контроллеров
	UPROPERTY()
	TMap<FName, FActiveMissionEntry> ActiveMissions;

	// ─── Вспомогательные методы ───────────────────────────────────────────────

	// Проверяет что scope нового envelope перекрывается со scope существующего
	bool ScopesOverlap(const FMissionEnvelopeScope& A, const FMissionEnvelopeScope& B) const;

	// Проверяет что два envelope имеют хотя бы один общий канал
	bool ChannelsOverlap(const FMissionEnvelope& A, const FMissionEnvelope& B) const;

	// Применить политику выхода из здания во время активной миссии (JobSpacePolicy)
	void ApplyEnvelopeExitPolicy(FName MissionId, const FMissionEnvelope& Envelope);

	// Применить политику завершения миссии (ExitPolicy.OnMissionCompleted)
	// bIsCompletion=true — записывает в FloorStateSnapshots согласно Policy для Completed,
	// для Failed/Abandoned Policy=Reset — только очищает MissionFloorSnapshots.
	void ApplyMissionCompletionPolicy(FName MissionId, const FMissionEnvelope& Envelope, EJobSpacePolicy Policy, EMissionEndReason EndReason);

	// Подписка на уведомления о покидании этажа (публикуется InteriorSubsystem)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Conditions|Envelope", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOutcomeConditionAsset> FloorLeavingCondition;

	FOutcomeHandlerHandle FloorLeavingHandle;

	// Обработчик: получает InteriorTransitionPayload (SourceFloor) и публикует FloorStateSave per mission
	void HandleFloorLeavingNotification(const FOutcomeEventBase& Outcome);

	void SubscribeFloorLeaving();
	void UnsubscribeFloorLeaving();

	FName ActiveMissionId;

	bool IsLoadComplete = true;

	UPROPERTY()
	TArray<FName> CompletedMissionHistory;
};
