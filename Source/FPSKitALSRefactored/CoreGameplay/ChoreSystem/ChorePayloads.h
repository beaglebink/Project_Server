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

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    bool bSuccess = false;

    UPROPERTY(BlueprintReadWrite, Category = "Chore")
    FChorePerformanceMetrics Performance;

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreCommandPayload* Setup(FName InChoreId)
    {
        ChoreId = InChoreId;
        return this;
    }

    UFUNCTION(BlueprintCallable, Category = "Chore")
    UChoreCommandPayload* SetupComplete(FName InChoreId, bool InSuccess, const FChorePerformanceMetrics& InPerf)
    {
        ChoreId = InChoreId;
        bSuccess = InSuccess;
        Performance = InPerf;
        return this;
    }
};