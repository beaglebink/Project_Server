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
    FDateTime Deadline;

    UPROPERTY()
    int32 AttemptCount = 0;

    UPROPERTY()
    bool bSucceeded = false;

    UPROPERTY()
    FChorePerformanceMetrics Performance;

    UPROPERTY()
    bool bRewardIssued = false;

    // Обработчик условия доступности
    FOutcomeHandlerHandle AvailabilityHandler;
};

USTRUCT()
struct FChoreHistoryEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FName ChoreId;

    UPROPERTY()
    bool bSucceeded = false;

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

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetAcceptedChoreIds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager|Query")
    TArray<FName> GetActiveChoreIds() const;

    // ---- Методы для условий истории (используются из Condition Assets) ----
    int32 GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const;
    float GetBestPerformance(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, const FString& MetricName) const;
    bool GetLastResult(FName ChoreId) const;

    // ---- Регистрация определений (вызывается внутри) ----
    void RegisterChoreDefinition(UChoreDefinition* Definition);

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- Внутренние методы управления (не публичные) ----
    void OfferChore(FName ChoreId);
    void AcceptChore(FName ChoreId);
    void StartChore(FName ChoreId);
    void CompleteChore(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance);
    void FailChore(FName ChoreId);
    void ExpireChore(FName ChoreId);
    void AbandonChore(FName ChoreId);
    void RetryChore(FName ChoreId);
    void UnlockChore(FName ChoreId);
    void RequestMissionChore(FName MissionId, FName ChoreId, int32 StepIndex);
    void ReportMissionChoreResult(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance, FName MissionId = NAME_None);

    // ---- Вспомогательные методы ----
    void LoadAllDefinitions();
    void RegisterAvailabilityHandler(UChoreDefinition* Definition);
    void UnregisterAvailabilityHandler(FName ChoreId);
    void UpdateChoreState(FName ChoreId, EChoreStatus NewStatus, bool bPublishEvent = true);
    void StartDeadlineTimer(FName ChoreId);
    void ClearDeadlineTimer(FName ChoreId);
    void GrantRewards(FName ChoreId);
    void AddHistoryEntry(FName ChoreId, bool bSucceeded, const FChorePerformanceMetrics& Performance);
    void EvaluateAllAvailability();

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

    // ---- Вспомогательные функции для создания условий ----
    UOutcomeConditionAsset* CreateSimpleChoreCondition(EOutcomeChore ChoreType);
    UOutcomeConditionAsset* CreateSimpleMissionCondition(EOutcomeMission MissionType);

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
};