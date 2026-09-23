#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "TerminalSubsystem.h"
#include "TerminalTaskPayload.generated.h"

// Payload for TerminalTask events (legacy, не связан с играми).
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UTerminalTaskPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "TerminalTask") FGuid TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "TerminalTask") FString TaskId;
    UPROPERTY(BlueprintReadWrite, Category = "TerminalTask") bool bSuccess = false;

    UFUNCTION(BlueprintCallable, Category = "TerminalTask")
    UTerminalTaskPayload* Setup(const FGuid& InTerminalId, const FString& InTaskId, bool bInSuccess)
    {
        TerminalId = InTerminalId; TaskId = InTaskId; bSuccess = bInSuccess; return this;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TerminalTask") FGuid   GetTerminalId() const { return TerminalId; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TerminalTask") FString GetTaskId()     const { return TaskId; }
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TerminalTask") bool    IsSuccess()     const { return bSuccess; }
};

// ============================================================================
// Payload'ы отчётов об играх терминала
// ============================================================================

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalActivityStagePayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") 
    FGuid TerminalId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") 
    FString ActivityId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") 
    FString StageId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game")
    ETerminalRecordStatus Status;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") 
    int32 Score = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") 
    FString Notes;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalActivityStagePayload* Setup(const FGuid& InTid, const FString& InAid,
        const FString& InSid, const ETerminalRecordStatus& InStatus, int32 InScore = 0, const FString& InNotes = TEXT(""))
    {
        TerminalId = InTid; 
        ActivityId = InAid; 
        StageId = InSid; 
        Status = InStatus;
        Score = InScore; 
        Notes = InNotes;
        return this;
    }
};

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalActivityResultPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") bool bSuccess = false;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") int32 TotalScore = 0;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ResultData;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalActivityResultPayload* Setup(const FGuid& InTid, const FString& InAid,
        bool bInSuccess, int32 InScore = 0, const FString& InData = TEXT(""))
    {
        TerminalId = InTid; ActivityId = InAid; bSuccess = bInSuccess;
        TotalScore = InScore; ResultData = InData; return this;
    }
};

// Удаление одной игры. Пустой ActivityId — все игры терминала.
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalRemoveActivityRecordPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalRemoveActivityRecordPayload* Setup(const FGuid& InTid, const FString& InAid = TEXT(""))
    {
        TerminalId = InTid; ActivityId = InAid; return this;
    }
};

// Payload исходящей нотификации — снимок записи целиком.
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalActivityRecordChangedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FTerminalActivityRecord Record;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalActivityRecordChangedPayload* Setup(const FGuid& InTid, const FString& InAid,
        const FTerminalActivityRecord& InRecord)
    {
        TerminalId = InTid; ActivityId = InAid; Record = InRecord; return this;
    }
};