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
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentStageIndex = 0;

    UPROPERTY(BlueprintReadOnly)
    FName CurrentStageKey;

    UPROPERTY(BlueprintReadOnly)
    bool IsStart = false;

    UPROPERTY(BlueprintReadOnly)
    bool IsExpired = false;

    // ---- Pause ----
    UPROPERTY(BlueprintReadOnly)
    bool bIsPaused = false;

    UPROPERTY(BlueprintReadOnly)
    FDateTime PauseStartTime;

    UPROPERTY(BlueprintReadOnly)
    FTimespan AccumulatedPauseTime;

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

    // ---- Публичные методы только для запросов состояния ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    EChoreStatus GetChoreStatus(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    UChoreDefinition* GetChoreDefinition(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    bool IsChoreAvailable(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    FChoreState GetState(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAvailableChoreIds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetActiveChoreIds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAcceptedChoreIds() const;

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

    // ---- НОВОЕ: игровое время ----
    // «Игровое» время: UtcNow минус всё время, проведённое вне игры
    // между сохранениями. Монотонно внутри сессии, непрерывно через save/load.
    // Все временные метки в подсистеме (History, AcceptTime и т.д.) пишутся
    // и сравниваются в этом времени.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Time")
    FDateTime GetGameTime() const;

    // ---- Расширенные запросы истории ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    bool WasChoreEverCompleted(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    EOutcomeChore GetLastOutcome(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    FChorePerformanceMetrics GetLastPerformance(FName ChoreId) const;

    int32 GetHistoryCountByResult(
        FName ChoreId,
        EChoreFamily Family,
        EChoreSubtype Subtype,
        bool bUseFamily,
        bool bUseSubtype,
        EOutcomeChore RequiredResult,
        bool bRequireSpecificResult) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetSuccessCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetExpireCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetAbandonCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetAnyFailureCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetTotalAttempts(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype,
        bool bUseFamily, bool bUseSubtype) const;

    float GetBestPerformanceFiltered(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, EChorePerformanceMetric Metric, bool bSucceededOnly) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    int32 GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    float GetBestPerformance(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, EChorePerformanceMetric Metric) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|History")
    bool GetLastResult(FName ChoreId) const;

    // ---- НОВОЕ: последний Timestamp в игровом времени ----
    // Возвращает Timestamp самой свежей записи истории, удовлетворяющей фильтру.
    // ChoreId == NAME_None   → любая хора.
    // AllowedResults         → какие результаты считать; пустой массив = любые.
    bool GetLatestHistoryTimestamp(FName ChoreId, EChoreFamily Family, bool bUseFamily, EChoreSubtype Subtype, bool bUseSubtype, const TArray<EOutcomeChore>& AllowedResults, FDateTime& OutTimestamp) const;

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

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool GetChoreStageDefinition(FName ChoreId, int32 StageIndex, FChoreStageDefinition& OutStage) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Stage")
    bool GetChoreCurrentStageDefinition(FName ChoreId, FChoreStageDefinition& OutStage) const;

    // ---- Награды ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Reward")
    bool WasRewardIssued(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, Category = "Chore Manager|Reward")
    void RequestRewardIssue(FName ChoreId);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Reward")
    bool GetChoreRewards(FName ChoreId, FChoreRewardSet& OutRewards) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Reward")
    bool CanShowRewardsBeforeAccept(FName ChoreId) const;

    void RegisterChoreDefinition(UChoreDefinition* Definition);

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

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

    void AdvanceChoreStage(FName ChoreId, int32 NewStageIndex, bool IsStart, FName NewStageKey);
    void ResetAttemptState(FChoreState& State);
    void RestartChore(FName ChoreId);

    void PauseChore(FName ChoreId);
    void ResumeChore(FName ChoreId);

private:
    void HandleAcceptRequest(const FOutcomeEventBase& Outcome);
    void HandleStartRequest(const FOutcomeEventBase& Outcome);
    void HandleCompleteRequest(const FOutcomeEventBase& Outcome);
    void HandleFailRequest(const FOutcomeEventBase& Outcome);
    void HandleExpireRequest(const FOutcomeEventBase& Outcome);
    void HandleAbandonRequest(const FOutcomeEventBase& Outcome);
    void HandleRetryRequest(const FOutcomeEventBase& Outcome);
    void HandleUnlockRequest(const FOutcomeEventBase& Outcome);
    void HandleEvent(const FOutcomeEventBase& Outcome);
    void HandleMissionRequest(const FOutcomeEventBase& Outcome);
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

    UOutcomeConditionAsset* CreateSimpleChoreCondition(EOutcomeChore ChoreType);
    UOutcomeConditionAsset* CreateSimpleMissionCondition(EOutcomeMission MissionType);

    void EnsureElapsedTimeRecorded(FChoreState& State) const;

    static float ExtractMetricValue(const FChorePerformanceMetrics& Perf, EChorePerformanceMetric Metric);
    static bool IsLowerBetterForMetric(EChorePerformanceMetric Metric);

    // ---- State ----
    UPROPERTY()
    TMap<FName, FChoreState> ActiveStates;

    UPROPERTY()
    TMap<FName, TObjectPtr<UChoreDefinition>> Definitions;

    UPROPERTY()
    TArray<FChoreHistoryEntry> History;

    UPROPERTY()
    bool bLoadComplete = true;

    // ---- НОВОЕ: накопленное offline-время ----
    // Сумма разниц между моментом сохранения и моментом загрузки по всем
    // циклам save/load. Вычитается из UtcNow в GetGameTime(), чтобы паузы
    // между сессиями не «тикали» в игре.
    UPROPERTY()
    FTimespan AccumulatedOfflineTime = FTimespan::Zero();

    FTimerManager* TimerManager = nullptr;
    TMap<FName, FTimerHandle> DeadlineTimers;

    // ---- НОВОЕ: pulse доступности ----
    // Раз в AvailabilityPulseIntervalSeconds напрямую дёргает
    // EvaluateAllAvailability, чтобы time-based условия перепроверялись
    // без внешних событий. Через EventBus не ходим — pulse нужен только нам.
    FTimerHandle AvailabilityPulseHandle;

    static constexpr float AvailabilityPulseIntervalSeconds = 5.0f;

    // ---- Хендлы подписок ----
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