// ChorePayloads.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "ChoreDefinition.h"
#include "ChorePayloads.generated.h"

// Payload для запроса от миссии на выполнение хоры как шага
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

// Payload для награды
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreRewardPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FChoreRewardSet Rewards;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreRewardPayload* Setup(FName InChoreId, const FChoreRewardSet& InRewards)
    {
        ChoreId = InChoreId;
        Rewards = InRewards;
        return this;
    }
};

// Payload для команд управления хорами (публикуется через EventBus)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreCommandPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    //UPROPERTY(BlueprintReadWrite, Category = "Chore")
    //bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
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
        //bSuccess = InSuccess;
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
    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FName ChoreId;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreReacceptPayload* Setup(FName InChoreId)
    {
        ChoreId = InChoreId;
        return this;
    }
};

// Payload для команды advance/нотификации о смене стадии
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UChoreStagePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    FName ChoreId;

    // При отправке: индекс новой стадии.
    // При нотификации: текущий индекс.
    UPROPERTY(BlueprintReadWrite, Category = "Chore|Stage")
    int32 StageIndex = 0;

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