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

    // Для внутреннего использования: обработчик условия доступности
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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- ISaveableSubsystem ----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("ChoreManager"); }
    virtual bool GetIsLoadComplete() const override { return bLoadComplete; }

    // ---- Публичный API для Blueprint ----
    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void OfferChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void AcceptChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void StartChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void CompleteChore(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void FailChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void ExpireChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void AbandonChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void RetryChore(FName ChoreId);

    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void UnlockChore(FName ChoreId);

    // Для миссий – запрос на выполнение хоры как шага
    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void RequestMissionChore(FName MissionId, FName ChoreId, int32 StepIndex);

    // Запись результата миссионной хоры (без изменения статуса задания)
    UFUNCTION(BlueprintCallable, Category = "Chore Manager")
    void ReportMissionChoreResult(FName ChoreId, bool bSuccess, const FChorePerformanceMetrics& Performance, FName MissionId = NAME_None);

    // ---- Запросы состояния ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager")
    EChoreStatus GetChoreStatus(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager")
    UChoreDefinition* GetChoreDefinition(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager")
    bool IsChoreAvailable(FName ChoreId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager")
    TArray<FName> GetAcceptedChoreIds() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Chore Manager")
    TArray<FName> GetActiveChoreIds() const;

    // ---- Для условий истории ----
    int32 GetHistoryCount(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, bool bSucceededOnly) const;
    float GetBestPerformance(FName ChoreId, EChoreFamily Family, EChoreSubtype Subtype, bool bUseFamily, bool bUseSubtype, const FString& MetricName) const;
    bool GetLastResult(FName ChoreId) const;

    // ---- Регистрация определений ----
    void RegisterChoreDefinition(UChoreDefinition* Definition);

protected:
    // ---- Внутренние методы ----
    void LoadAllDefinitions();
    void RegisterAvailabilityHandler(UChoreDefinition* Definition);
    void UnregisterAvailabilityHandler(FName ChoreId);
    void UpdateChoreState(FName ChoreId, EChoreStatus NewStatus, bool bPublishEvent = true);
    void StartDeadlineTimer(FName ChoreId);
    void ClearDeadlineTimer(FName ChoreId);
    void GrantRewards(FName ChoreId);
    void AddHistoryEntry(FName ChoreId, bool bSucceeded, const FChorePerformanceMetrics& Performance);
    void EvaluateAllAvailability();

    // ---- Обработчики событий ----
    void HandleEvent(const FOutcomeEventBase& Outcome);
    void HandleMissionRequest(const FOutcomeEventBase& Outcome);
    void HandleChoreCompletion(const FOutcomeEventBase& Outcome);
    //void HandleCheckRequest(const FOutcomeEventBase& Outcome);

    // ---- Состояние ----
    UPROPERTY()
    TMap<FName, FChoreState> ActiveStates;

    UPROPERTY()
    TMap<FName, TObjectPtr<UChoreDefinition>> Definitions;

    UPROPERTY()
    TArray<FChoreHistoryEntry> History;

    UPROPERTY()
    bool bLoadComplete = true;

    // ---- Таймеры ----
    FTimerManager* TimerManager = nullptr;
    TMap<FName, FTimerHandle> DeadlineTimers;

    // ---- Хендлы подписок ----
    FOutcomeHandlerHandle GlobalEventHandler;
    FOutcomeHandlerHandle MissionRequestHandler;
    FOutcomeHandlerHandle ChoreCompletionHandler;
    FOutcomeHandlerHandle CheckRequestHandler;

    // ---- Условия для подписок ----
    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> GlobalEventCondition;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> MissionRequestCondition;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> ChoreCompletionCondition;

    UPROPERTY()
    TObjectPtr<UOutcomeConditionAsset> CheckRequestCondition;

    // ---- Вспомогательные структуры ----
    struct FPendingDefinitionLoad
    {
        FName ChoreId;
        TSoftObjectPtr<UChoreDefinition> DefinitionPtr;
    };
    TArray<FPendingDefinitionLoad> PendingDefinitions;
};