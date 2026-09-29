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

USTRUCT(BlueprintType)
struct FChoreState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ChoreId;

    UPROPERTY(BlueprintReadOnly)
    EChoreStatus Status = EChoreStatus::Unavailable;

    UPROPERTY(BlueprintReadOnly)
    FDateTime AcceptTime;

    UPROPERTY(BlueprintReadOnly)
    FDateTime StartTime;

    UPROPERTY(BlueprintReadOnly)
    FDateTime Deadline;

    UPROPERTY(BlueprintReadOnly)
    int32 AttemptCount = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bSucceeded = false;

    UPROPERTY(BlueprintReadOnly)
    FChorePerformanceMetrics Performance;

    UPROPERTY(BlueprintReadOnly)
    bool bRewardIssued = false;

    // ---- Stages ----
    // ---- Стадии ----
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentStageIndex = 0;

    UPROPERTY(BlueprintReadOnly)
    FName CurrentStageKey;

    UPROPERTY(BlueprintReadOnly)
    bool IsStart = true;

    UPROPERTY(BlueprintReadOnly)
    bool IsExpired = true;

    // ---- Pause ----
    // ---- Пауза ----
    UPROPERTY(BlueprintReadOnly)
    bool bIsPaused = false;

    UPROPERTY(BlueprintReadOnly)
    FDateTime PauseStartTime;

    UPROPERTY(BlueprintReadOnly)
    FTimespan AccumulatedPauseTime;

    // Remaining deadline at the moment of pause. 0 = there was no deadline.
    // Остаток дедлайна на момент паузы. 0 = дедлайна не было.
    UPROPERTY(BlueprintReadOnly)
    FTimespan PausedDeadlineRemaining = FTimespan::Zero();

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

    // ---- Public methods for state queries only (do not change state) ----
    // ---- Публичные методы только для запросов состояния (не изменяют состояние) ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    EChoreStatus GetChoreStatus(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    UChoreDefinition* GetChoreDefinition(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    bool IsChoreAvailable(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    FChoreState GetState(FName ChoreId) const;

    // ---- Returns a list of identifiers of available chores ----
    // ---- Возвращает список идентификаторов доступных заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAvailableChoreIds() const;

    // ---- Returns a list of identifiers of active chores ----
    // ---- Возвращает список идентификаторов активных заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetActiveChoreIds() const;

    // ---- Returns a list of identifiers of accepted chores ----
    // ---- Возвращает список идентификаторов принятых заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAcceptedChoreIds() const;

    // ---- Returns a list of identifiers of all successfully completed chores ----
    // ---- Возвращает список идентификаторов всех успешно выполненных заданий ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetSucceededChoreIds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    UChoreDefinition* GetChoreDefinitionByDisplayName(const FText& DisplayName) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetChoreIdsByDisplayName(const FText& DisplayName) const;

    // ---- Execution time ----
    // ---- Время выполнения ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query|Time")
    float GetChoreElapsedTime(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query|Time")
    FDateTime GetChoreStartTime(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, Category = "Chore Manager|Time")
    void SetChoreElapsedTime(FName ChoreId, float ElapsedSeconds);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager|Time")
    void SetChoreStartTime(FName ChoreId, FDateTime InStartTime);

    // ---- Extended history queries ----
    // ---- Расширенные запросы истории ----

// Has the chore ever been successfully completed?
// Было ли задание когда-либо успешно выполнено?
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    bool WasChoreEverCompleted(FName ChoreId) const;

    // Last result of the chore (Default if there are no entries)
    // Последний результат задания (Default, если записей нет)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    EOutcomeChore GetLastOutcome(FName ChoreId) const;

    // Last recorded performance of the chore
    // Последняя зафиксированная производительность задания
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    FChorePerformanceMetrics GetLastPerformance(FName ChoreId) const;

    // Universal counter: filter by (ChoreId | Family | Subtype) + optionally by Result.
    // bRequireSpecificResult = false → counts all entries matching the filter (TotalAttempts).
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

    // How many times the chore/family/subtype was successfully completed
    // Сколько раз задание/семейство/подтип было успешно завершено
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetSuccessCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // How many times it failed (FailRequest)
    // Сколько раз провалено (FailRequest)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // How many times it expired by timeout (ExpireRequest)
    // Сколько раз истекло по таймауту (ExpireRequest)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetExpireCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // How many times it was abandoned by the player (AbandonRequest)
    // Сколько раз отменено игроком (AbandonRequest)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetAbandonCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Any failure = Fail + Expire + Abandon
    // Любая неудача = Fail + Expire + Abandon
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetAnyFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Total number of attempts (all results) — denominator for win-rate
    // Общее число попыток (все результаты) — знаменатель для win-rate
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetTotalAttempts(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    // Best performance with an optional "successful only" filter.
    // We leave the existing GetBestPerformance untouched so as not to break Condition Assets.
    // Лучшая производительность с опциональным фильтром «только успешные».
    // Существующий GetBestPerformance не трогаем, чтобы не ломать Condition Assets.
    float GetBestPerformanceFiltered(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, EChorePerformanceMetric Metric, bool bSucceededOnly) const;

    // ---- Methods for history conditions (used from Condition Assets) ----
    // ---- Методы для условий истории (используются из Condition Assets) ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    // ---- Best performance for the selected metric ----
    // The optimization direction is set by the enum itself (see IsLowerBetterForMetric).
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

    // Returns the authored stage definition (nullptr if the stage does not exist).
    // Возвращает authored-определение стадии (nullptr, если стадии нет).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool GetChoreStageDefinition(FName ChoreId, int32 StageIndex, FChoreStageDefinition& OutStage) const;

    // Returns the definition of the current chore stage.
    // Возвращает определение текущей стадии хоры.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool GetChoreCurrentStageDefinition(FName ChoreId, FChoreStageDefinition& OutStage) const;

    // ---- Rewards ----
    // ---- Награды ----

    // Has the reward for the chore already been sent to subsystems?
    // false — if the chore has not yet been successfully completed, was abandoned,
    // or finished unsuccessfully.
    // Была ли награда за хору уже отправлена подсистемам?
    // false — если хора ещё не завершалась успешно, была отменена
    // или завершилась неудачно.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Reward")
    bool WasRewardIssued(FName ChoreId) const;

    // Manual request to issue the reward. Idempotent: if the reward has already
    // been sent or the chore is not in the Succeeded status — does nothing.
    // Used by UI/dialogs when a "grant reward by button" action is needed
    // after the chore has already been completed.
    // Ручной запрос на выдачу награды. Идемпотентен: если награда уже
    // была отправлена или хора не в статусе Succeeded — ничего не делает.
    // Используется UI/диалогами, когда нужно «выдать награду по кнопке»
    // уже после завершения хоры.
    UFUNCTION(BlueprintCallable, Category = "Chore Manager|Reward")
    void RequestRewardIssue(FName ChoreId);

    // ---- Rewards ----
    // ---- Награды ----

    // Returns the set of rewards for the chore if the author allowed showing
    // it in advance (bShowRewardBeforeAccept == true).
    //
    // If the flag is false — returns false and OutRewards remains empty.
    // In that case the player learns about the reward only at the moment of actual
    // granting, via the ChoreRewardGranted event (UChoreRewardPayload).
    // Возвращает набор наград за хору, если автор разрешил показывать
    // его заранее (bShowRewardBeforeAccept == true).
    //
    // Если флаг false — вернёт false и OutRewards останется пустым.
    // Игрок в этом случае узнаёт о награде только в момент фактической
    // выдачи, через событие ChoreRewardGranted (UChoreRewardPayload).
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Reward")
    bool GetChoreRewards(FName ChoreId, FChoreRewardSet& OutRewards) const;

    // Helper: is advance reward display allowed for this chore.
    // Хелпер: разрешён ли предварительный показ наград у этой хоры.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Reward")
    bool CanShowRewardsBeforeAccept(FName ChoreId) const;

    // ---- Definition registration (called internally) ----
    // ---- Регистрация определений (вызывается внутри) ----
    void RegisterChoreDefinition(UChoreDefinition* Definition);

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- Internal management methods (not public) ----
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

    // ---- Helper methods ----
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

    // Accepts an advance from the mini-game. Returns true if the state changed.
    // Принимает advance от миниигры. Возвращает true, если состояние изменено.
    void AdvanceChoreStage(FName ChoreId, int32 NewStageIndex, bool IsStart, FName NewStageKey);

    // Resets the "attempt progress" (stages, pauses, metrics, time).
    // Does not touch status, AttemptCount, AcceptTime, bRewardIssued.
    // Сбрасывает «прогресс попытки» (стадии, паузы, метрики, время).
    // Не трогает статус, AttemptCount, AcceptTime, bRewardIssued.
    void ResetAttemptState(FChoreState& State);

    // Full attempt restart procedure: stops the deadline timer and
    // resets progress. Does not change status and does not publish events —
    // the external signal is the RetryRequest itself, from which we are called.
    // Полная процедура рестарта попытки: гасит таймер дедлайна и
    // сбрасывает прогресс. Не меняет статус и не публикует события —
    // сигналом наружу служит сам RetryRequest, из которого мы вызваны.
    void RestartChore(FName ChoreId);

    void PauseChore(FName ChoreId);
    void ResumeChore(FName ChoreId);

private:
    // ---- Command handlers (subscribed to EventBus) ----
    // ---- Обработчики команд (подписаны на EventBus) ----
    void HandleAcceptRequest(const FOutcomeEventBase& Outcome);
    void HandleStartRequest(const FOutcomeEventBase& Outcome);
    void HandleCompleteRequest(const FOutcomeEventBase& Outcome);
    void HandleFailRequest(const FOutcomeEventBase& Outcome);
    void HandleExpireRequest(const FOutcomeEventBase& Outcome);
    void HandleAbandonRequest(const FOutcomeEventBase& Outcome);
    void HandleRetryRequest(const FOutcomeEventBase& Outcome);
    void HandleUnlockRequest(const FOutcomeEventBase& Outcome);
    // for global availability conditions
    // для глобальных условий доступности
    void HandleEvent(const FOutcomeEventBase& Outcome);
    // for ChoreMissionRequest
    // для ChoreMissionRequest
    void HandleMissionRequest(const FOutcomeEventBase& Outcome);
    // for completion of regular chores
    // для завершения обычных хор
    void HandleChoreCompletion(const FOutcomeEventBase& Outcome);
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

    // ---- Helper functions for creating conditions ----
    // ---- Вспомогательные функции для создания условий ----
    UOutcomeConditionAsset* CreateSimpleChoreCondition(EOutcomeChore ChoreType);
    UOutcomeConditionAsset* CreateSimpleMissionCondition(EOutcomeMission MissionType);

    // Fills State.Performance.CompletionTimeSeconds if it is not yet set
    // Заполняет State.Performance.CompletionTimeSeconds, если оно ещё не задано
    void EnsureElapsedTimeRecorded(FChoreState& State) const;

    // Extracts the metric value from a history entry.
    // Извлекает значение метрики из записи истории.
    static float ExtractMetricValue(const FChorePerformanceMetrics& Perf, EChorePerformanceMetric Metric);

    // true for metrics where "better" = smaller (time, mistakes).
    // true для метрик, где «лучше» = меньше (время, ошибки).
    static bool IsLowerBetterForMetric(EChorePerformanceMetric Metric);

    // ---- State ----
    // ---- Состояние ----
    UPROPERTY()
    TMap<FName, FChoreState> ActiveStates;

    UPROPERTY()
    TMap<FName, TObjectPtr<UChoreDefinition>> Definitions;

    UPROPERTY()
    TArray<FChoreHistoryEntry> History;

    UPROPERTY()
    bool bLoadComplete = true;

    FTimerManager* TimerManager = nullptr;
    TMap<FName, FTimerHandle> DeadlineTimers;

    // ---- Event subscription handles ----
    // ---- Хендлы подписок на события ----
    FOutcomeHandlerHandle GlobalEventHandler;
    FOutcomeHandlerHandle ChoreCompletionHandler;
    FOutcomeHandlerHandle MissionRequestHandler;

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

    // ---- Conditions for subscriptions ----
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