#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ISaveableSubsystem.h"
#include "ChoreEnums.h"
#include "ChoreDefinition.h"
#include "ChorePayloads.h"
#include "OutcomeEventBase.h"
#include "EventBusSubsystem.h"
#include "ChoreManagerSubsystem.generated.h"

USTRUCT()
struct FChoreState
{
    GENERATED_BODY()

    UPROPERTY()
    FName ChoreId;

    UPROPERTY()
    EChoreStatus Status = EChoreStatus::Unavailable;

    UPROPERTY()
    FDateTime AcceptTime;

    UPROPERTY()
    FDateTime StartTime;

    UPROPERTY()
    FDateTime Deadline;

    UPROPERTY()
    int32 AttemptCount = 0;

    UPROPERTY()
    bool bSucceeded = false;

    UPROPERTY()
    FChorePerformanceMetrics Performance;

    UPROPERTY()
    bool bRewardIssued = false;

    // ---- Стадии ----
    UPROPERTY()
    int32 CurrentStageIndex = 0;

    UPROPERTY()
    FName CurrentStageKey;

    // ---- Pause ----
    UPROPERTY()
    bool bIsPaused = false;

    UPROPERTY()
    FDateTime PauseStartTime;

    UPROPERTY()
    FTimespan AccumulatedPauseTime;

    FOutcomeHandlerHandle AvailabilityHandler;
    FOutcomeHandlerHandle ReactivationHandler;
};

USTRUCT()
struct FChoreHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FName ChoreId;

    UPROPERTY()
    EOutcomeChore Result = EOutcomeChore::Default;

    UPROPERTY()
    FChorePerformanceMetrics Performance;

    UPROPERTY()
    FDateTime Timestamp;
};

UCLASS()
class FPSKITALSREFACTORED_API UChoreManagerSubsystem : public UGameInstanceSubsystem, public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    // ---- ISaveableSubsystem ----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("ChoreManager"); }
    virtual bool GetIsLoadComplete() const override { return bLoadComplete; }

    // ---- Публичные методы только для запросов состояния (не изменяют состояние) ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    EChoreStatus GetChoreStatus(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    UChoreDefinition* GetChoreDefinition(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    bool IsChoreAvailable(FName ChoreId) const;

    // ---- Возвращает список идентификаторов доступных заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAvailableChoreIds() const;

    // ---- Возвращает список идентификаторов активных заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetActiveChoreIds() const;

    // ---- Возвращает список идентификаторов принятых заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAcceptedChoreIds() const;

    // ---- Возвращает список идентификаторов всех успешно выполненных заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetSucceededChoreIds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    UChoreDefinition* GetChoreDefinitionByDisplayName(const FText& DisplayName) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetChoreIdsByDisplayName(const FText& DisplayName) const;

    // ---- Время выполнения ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query|Time")
    float GetChoreElapsedTime(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query|Time")
    FDateTime GetChoreStartTime(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, Category = "Chore Manager|Time")
    void SetChoreElapsedTime(FName ChoreId, float ElapsedSeconds);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager|Time")
    void SetChoreStartTime(FName ChoreId, FDateTime InStartTime);

    // ---- Расширенные запросы истории ----

// Было ли задание когда-либо успешно выполнено?
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    bool WasChoreEverCompleted(FName ChoreId) const;

    // Последний результат задания (Default, если записей нет)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    EOutcomeChore GetLastOutcome(FName ChoreId) const;

    // Последняя зафиксированная производительность задания
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    FChorePerformanceMetrics GetLastPerformance(FName ChoreId) const;

    // Универсальный счётчик: фильтр по (ChoreId | Family | Subtype) + опционально по Result.
    // bRequireSpecificResult = false → считает все записи, попадающие под фильтр (TotalAttempts).
    int32 GetHistoryCountByResult(
        FName ChoreId,
        EChoreFamily Family,
        EChoreSubtype Subtype,
        bool bUseFamily,
        bool bUseSubtype,
        EOutcomeChore RequiredResult,
        bool bRequireSpecificResult) const;

    // Сколько раз задание/семейство/подтип было успешно завершено
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetSuccessCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Сколько раз провалено (FailRequest)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Сколько раз истекло по таймауту (ExpireRequest)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetExpireCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Сколько раз отменено игроком (AbandonRequest)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetAbandonCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Любая неудача = Fail + Expire + Abandon
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetAnyFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Общее число попыток (все результаты) — знаменатель для win-rate
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetTotalAttempts(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Лучшая производительность с опциональным фильтром «только успешные».
    // Существующий GetBestPerformance не трогаем, чтобы не ломать Condition Assets.
    float GetBestPerformanceFiltered(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, EChorePerformanceMetric Metric, bool bSucceededOnly) const;

    // ---- Методы для условий истории (используются из Condition Assets) ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    // ---- Лучшая производительность по выбранной метрике ----
    // Направление оптимизации задаётся самим enum (см. IsLowerBetterForMetric).
    float GetBestPerformance(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, EChorePerformanceMetric Metric) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    bool GetLastResult(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    int32 GetChoreCurrentStage(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    int32 GetChoreTotalStages(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    FName GetChoreCurrentStageKey(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool IsChorePaused(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool IsChoreMultiStage(FName ChoreId) const;

    // Возвращает authored-определение стадии (nullptr, если стадии нет).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool GetChoreStageDefinition(FName ChoreId, int32 StageIndex, FChoreStageDefinition& OutStage) const;

    // Возвращает определение текущей стадии хоры.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool GetChoreCurrentStageDefinition(FName ChoreId, FChoreStageDefinition& OutStage) const;

    // ---- Регистрация определений (вызывается внутри) ----
    void RegisterChoreDefinition(UChoreDefinition* Definition);

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- Внутренние методы управления (не публичные) ----
    void OfferChore(FName ChoreId);
    void AcceptChore(FName ChoreId);
    void StartChore(FName ChoreId);
    void CompleteChore(FName ChoreId, const FChorePerformanceMetrics& Performance);
    void FailChore(FName ChoreId, const FChorePerformanceMetrics& Performance);
    void ExpireChore(FName ChoreId);
    void AbandonChore(FName ChoreId);
    void RetryChore(FName ChoreId);
    void UnlockChore(FName ChoreId);
    void RevokeChore(FName ChoreId);
    void RequestMissionChore(FName MissionId, FName ChoreId, int32 StepIndex);
    void ReportMissionChoreResult(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance, FName MissionId = NAME_None);

    // ---- Вспомогательные методы ----
    void LoadAllDefinitions();
    void RegisterAvailabilityHandler(UChoreDefinition* Definition);
    void UnregisterAvailabilityHandler(FName ChoreId);
    void UpdateChoreState(FName ChoreId, EChoreStatus NewStatus, bool bPublishEvent = true, bool bReactivated = false);
    void StartDeadlineTimer(FName ChoreId);
    void ClearDeadlineTimer(FName ChoreId);
    void GrantRewards(FName ChoreId);
    void AddHistoryEntry(FName ChoreId, EOutcomeChore Result, const FChorePerformanceMetrics& Performance);
    void EvaluateAllAvailability();
    void RegisterReactivationHandler(UChoreDefinition* Definition);
    void UnregisterReactivationHandler(FName ChoreId);

    // Принимает advance от миниигры. Возвращает true, если состояние изменено.
    void AdvanceChoreStage(FName ChoreId, int32 NewStageIndex, FName NewStageKey);

    void PauseChore(FName ChoreId);
    void ResumeChore(FName ChoreId);

private:
    // ---- Обработчики команд (подписаны на EventBus) ----
    void HandleAcceptRequest(const FOutcomeEventBase& Outcome);
    void HandleStartRequest(const FOutcomeEventBase& Outcome);
    void HandleCompleteRequest(const FOutcomeEventBase& Outcome);
    void HandleFailRequest(const FOutcomeEventBase& Outcome);
    void HandleExpireRequest(const FOutcomeEventBase& Outcome);
    void HandleAbandonRequest(const FOutcomeEventBase& Outcome);
    void HandleRetryRequest(const FOutcomeEventBase& Outcome);
    void HandleUnlockRequest(const FOutcomeEventBase& Outcome);
    void HandleEvent(const FOutcomeEventBase& Outcome);          // для глобальных условий доступности
    void HandleMissionRequest(const FOutcomeEventBase& Outcome); // для ChoreMissionRequest
    void HandleChoreCompletion(const FOutcomeEventBase& Outcome); // для завершения обычных хор
    void HandleRegisterChoreRequest(const FOutcomeEventBase& Outcome);
    void HandleUnregisterChoreRequest(const FOutcomeEventBase& Outcome);
    void HandleReacceptRequest(const FOutcomeEventBase& Outcome);
    void HandleAdvanceStageRequest(const FOutcomeEventBase& Outcome);
    void HandlePauseRequest(const FOutcomeEventBase& Outcome);
    void HandleResumeRequest(const FOutcomeEventBase& Outcome);

    FOutcomeHandlerHandle AdvanceStageRequestHandler;
    FOutcomeHandlerHandle PauseRequestHandler;
    FOutcomeHandlerHandle ResumeRequestHandler;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> AdvanceStageRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> PauseRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> ResumeRequestCondition;

    // ---- Вспомогательные функции для создания условий ----
    UOutcomeConditionAsset* CreateSimpleChoreCondition(EOutcomeChore ChoreType);
    UOutcomeConditionAsset* CreateSimpleMissionCondition(EOutcomeMission MissionType);

    // Заполняет State.Performance.CompletionTimeSeconds, если оно ещё не задано
    void EnsureElapsedTimeRecorded(FChoreState& State) const;

    // Извлекает значение метрики из записи истории.
    static float ExtractMetricValue(const FChorePerformanceMetrics& Perf, EChorePerformanceMetric Metric);

    // true для метрик, где «лучше» = меньше (время, ошибки).
    static bool IsLowerBetterForMetric(EChorePerformanceMetric Metric);

    // ---- Состояние ----
    UPROPERTY()
    TMap<FName, FChoreState> ActiveStates;

    UPROPERTY()
    TMap<FName, TObjectPtr<UChoreDefinition>> Definitions;

    UPROPERTY()
    TArray<FChoreHistoryEntry> History;

    UPROPERTY()
    bool bLoadComplete = true;
    /*
    UPROPERTY()
    TArray<FShoreNameStatus> NotPublishState;
    */
    FTimerManager* TimerManager = nullptr;
    TMap<FName, FTimerHandle> DeadlineTimers;

    // ---- Хендлы подписок на события ----
    FOutcomeHandlerHandle GlobalEventHandler;           // для глобальных условий
    FOutcomeHandlerHandle ChoreCompletionHandler;       // для завершения обычных хор
    FOutcomeHandlerHandle MissionRequestHandler;        // для запросов от миссий

    FOutcomeHandlerHandle AcceptRequestHandler;
    FOutcomeHandlerHandle StartRequestHandler;
    FOutcomeHandlerHandle CompleteRequestHandler;
    FOutcomeHandlerHandle FailRequestHandler;
    FOutcomeHandlerHandle ExpireRequestHandler;
    FOutcomeHandlerHandle AbandonRequestHandler;
    FOutcomeHandlerHandle RetryRequestHandler;
    FOutcomeHandlerHandle UnlockRequestHandler;
    FOutcomeHandlerHandle RegisterChoreRequestHandler;
    FOutcomeHandlerHandle UnregisterChoreRequestHandler;
    FOutcomeHandlerHandle ReacceptRequestHandler;

    // ---- Условия для подписок ----
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> GlobalEventCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> ChoreCompletionCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> MissionRequestCondition;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> AcceptRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> StartRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> CompleteRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> FailRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> ExpireRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> AbandonRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> RetryRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> UnlockRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> RegisterChoreRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> UnregisterChoreRequestCondition;
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> ReacceptRequestCondition;
};