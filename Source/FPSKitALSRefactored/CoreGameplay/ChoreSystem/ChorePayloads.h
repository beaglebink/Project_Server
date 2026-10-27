// ChorePayloads.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "ChoreDefinition.h"
#include "ChorePayloads.generated.h"

// Payload for a mission request to perform a chore as a step
// Payload для запроса от миссии на выполнение хоры как шага
/*
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreMissionRequestPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Mission")
    FName MissionId;

    UPROPERTY(BlueprintReadWrite, Category = "Mission")
    FName ChoreId;

    UPROPERTY(BlueprintReadWrite, Category = "Mission")
    int32 MissionStepIndex = 0;

    UFUNCTION(BlueprintCallable, Category = "Mission")
    UChoreMissionRequestPayload* Setup(FName InMissionId, FName InChoreId, int32 InStepIndex)
    {
        MissionId = InMissionId;
        ChoreId = InChoreId;
        MissionStepIndex = InStepIndex;
        return this;
    }
};
*/

// Payload for the result of a chore (including mission chores)
// Payload для результата выполнения хоры (в том числе миссионной)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreResultPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    bool bSucceeded = false;

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FChorePerformanceMetrics Performance;

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FName MissionId;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreResultPayload* Setup(FName InChoreId, bool InSuccess, const FChorePerformanceMetrics& InPerf, FName InMissionId = NAME_None)
    {
        ChoreId = InChoreId;
        bSucceeded = InSuccess;
        Performance = InPerf;
        MissionId = InMissionId;
        return this;
    }
};

// Payload for chore control commands (published via EventBus)
// Payload для команд управления хорами (публикуется через EventBus)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chore")
    FChorePerformanceMetrics Performance;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreCommandPayload* Setup(FName InChoreId)
    {
        ChoreId = InChoreId;
        return this;
    }

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreCommandPayload* SetupComplete(FName InChoreId, const FChorePerformanceMetrics& InPerf)
    {
        ChoreId = InChoreId;
        Performance = InPerf;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreRegisterPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    TObjectPtr<UChoreDefinition> Definition;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreRegisterPayload* Setup(UChoreDefinition* InDefinition)
    {
        Definition = InDefinition;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreUnregisterPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    // If true – forcibly complete the task if it is active
    // Если true – принудительно завершить задание, если оно активно
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    bool bForceRemove = false;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreUnregisterPayload* Setup(FName InChoreId, bool bForce = false)
    {
        ChoreId = InChoreId;
        bForceRemove = bForce;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreReacceptPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreReacceptPayload* Setup(FName InChoreId)
    {
        ChoreId = InChoreId;
        return this;
    }
};

// Payload for the advance command / stage change notification
// Payload для команды advance/нотификации о смене стадии
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreStagePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    FName ChoreId;

    // When sending: index of the new stage.
    // При отправке: индекс новой стадии.
    // On notification: current index.
    // При нотификации: текущий индекс.
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    int32 StageIndex = 0;

    // Filled in by the manager on notification.
    // Заполняется менеджером при нотификации.
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    FName StageKey;

    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    int32 TotalStages = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    FText StageDisplayName;

    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    bool IsStart = false;

    UFUNCTION(BlueprintCallable, Category = "Chore|Stage")
    UChoreStagePayload* Setup(FName InChoreId, int32 InStageIndex, bool InIsStart)
    {
        ChoreId = InChoreId;
        StageIndex = InStageIndex;
        IsStart = InIsStart;
        return this;
    }
};

// Payload for reporting when a task is paused and unpaused
// Payload для отчета о постановке задачи на паузу и о снятии с паузы
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChorePauseReportPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chore|Reward")
    FName ChoreId;

    UFUNCTION(BlueprintCallable, Category = "Chore|Reward")
    UChorePauseReportPayload* Setup(FName InChoreId)
    {
        ChoreId = InChoreId;
        return this;
    }
};

// Payload for the chore reward grant signal.
// Published after a successful chore completion (Succeeded),
// when the manager asks owning subsystems to grant their portions.
// Payload для сигнала о выдаче награды за хору.
// Публикуется после успешного завершения хоры (Succeeded),
// когда менеджер просит owning-подсистемы выдать свои порции.
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreRewardPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Reward")
    FName ChoreId;

    // Display name of the chore (for UI: "Reward for package delivery").
    // Отображаемое имя хоры (для UI: "Награда за доставку посылки").
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Reward")
    FText ChoreDisplayName;

    // The reward set itself. The same instance goes to all owning subsystems;
    // each takes its own portion (Money, ItemIds, Experience).
    // Сам набор награды. Один и тот же экземпляр уходит во все owning-подсистемы;
    // каждая берёт свою часть (Money, ItemIds, Experience).
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Reward")
    FChoreRewardSet Rewards;

    UFUNCTION(BlueprintCallable, Category = "Chore|Reward")
    UChoreRewardPayload* Setup(FName InChoreId, const FText& InDisplayName, const FChoreRewardSet& InRewards)
    {
        ChoreId = InChoreId;
        ChoreDisplayName = InDisplayName;
        Rewards = InRewards;
        return this;
    }
};

// ============================================================================
// Deferred evaluation
// ============================================================================

// Manager → Actor: "сообщи результат выполнения".
// Manager публикует это в ответ на внешнюю команду DeferredEvaluationRequest.
// Actor подписан на EOutcomeChore::DeferredEvaluationQuery и обязан вернуть
// DeferredResultReport с тем же Token.
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreDeferredQueryPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    FName ChoreId;

    // Токен корреляции. Actor должен вернуть его же в DeferredResultReport,
    // чтобы Manager принял отчёт как ответ на этот конкретный query.
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    int32 Token = 0;

    UFUNCTION(BlueprintCallable, Category = "Chore|Deferred")
    UChoreDeferredQueryPayload* Setup(FName InChoreId, int32 InToken)
    {
        ChoreId = InChoreId;
        Token = InToken;
        return this;
    }
};

// Actor → Manager: результат выполнения.
//   * ответ на DeferredEvaluationQuery — Token != 0, bFromEndPlay = false;
//   * unsolicited из EndPlay           — Token == 0, bFromEndPlay = true.
// Manager → Actor: replay сохранённого результата (Token == 0, bFromEndPlay == false).
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreDeferredResultPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    FName ChoreId;

    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    bool bSucceeded = false;

    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    FChorePerformanceMetrics Performance;

    // Токен из DeferredEvaluationQuery. 0 = unsolicited (EndPlay) или replay.
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    int32 Token = 0;

    // true — отчёт отправлен из EndPlay. Manager принимает такие отчёты
    // без ожидания pending-запроса.
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Deferred")
    bool bFromEndPlay = false;

    UFUNCTION(BlueprintCallable, Category = "Chore|Deferred")
    UChoreDeferredResultPayload* Setup(
        FName InChoreId,
        bool InSuccess,
        const FChorePerformanceMetrics& InPerf,
        int32 InToken = 0,
        bool bInFromEndPlay = false)
    {
        ChoreId = InChoreId;
        bSucceeded = InSuccess;
        Performance = InPerf;
        Token = InToken;
        bFromEndPlay = bInFromEndPlay;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreActorDataPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Data")
    FName ChoreId;

    // УНИКАЛЬНЫЙ КЛЮЧ АКТОРА (например, имя объекта на сцене)
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Data")
    FName DataKey;

    UPROPERTY(BlueprintReadWrite, Category = "Chore|Data")
    FString DataJson;

    UFUNCTION(BlueprintCallable, Category = "Chore|Data")
    UChoreActorDataPayload* Setup(FName InChoreId, FName InDataKey, const FString& InDataJson)
    {
        ChoreId = InChoreId;
        DataKey = InDataKey;
        DataJson = InDataJson;
        return this;
    }
};
