#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "TerminalSubsystem.h"
#include "TerminalTaskPayload.generated.h"

// ============================================================================
// Legacy: TerminalTask (not related to games)
// Legacy: TerminalTask (не связан с играми)
// ============================================================================
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UTerminalTaskPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "TerminalTask") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "TerminalTask") FString TaskId;
    UPROPERTY(BlueprintReadWrite, Category = "TerminalTask") bool    bSuccess = false;

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
// INCOMING commands: reports from terminals
// ВХОДЯЩИЕ команды: отчёты от терминалов
// ============================================================================

// ---- Game start ----
// ---- Старт игры ----

/*
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameStartedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameStartedPayload* Setup(const FGuid& InTid, const FString& InAid)
    {
        TerminalId = InTid; ActivityId = InAid; return this;
    }
};

// ---- Stage finished (success / failure / skip) ----
// ---- Этап завершён (успех / провал / пропуск) ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameStageFinishedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString StageId;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game")
    ETerminalStageResult Result = ETerminalStageResult::Completed;

    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") int32   Score = 0;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString Notes;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameStageFinishedPayload* Setup(const FGuid& InTid, const FString& InAid,
        const FString& InSid,
        ETerminalStageResult InResult = ETerminalStageResult::Completed,
        int32 InScore = 0, const FString& InNotes = TEXT(""))
    {
        TerminalId = InTid; ActivityId = InAid; StageId = InSid;
        Result = InResult; Score = InScore; Notes = InNotes;
        return this;
    }
};

// ---- Game completed ----
// ---- Игра завершена ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameCompletedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") bool    bSuccess = false;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") int32   TotalScore = 0;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ResultData;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameCompletedPayload* Setup(const FGuid& InTid, const FString& InAid,
        bool bInSuccess, int32 InScore = 0, const FString& InData = TEXT(""))
    {
        TerminalId = InTid; ActivityId = InAid; bSuccess = bInSuccess;
        TotalScore = InScore; ResultData = InData; return this;
    }
};
*/
// ---- Removing a record (empty ActivityId — all games of the terminal) ----
// ---- Удаление записи (пустой ActivityId — все игры терминала) ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalRemoveActivityRecordPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalRemoveActivityRecordPayload* Setup(const FGuid& InTid, const FString& InAid = TEXT(""))
    {
        TerminalId = InTid; ActivityId = InAid; return this;
    }
};

// ============================================================================
// OUTGOING notifications: subsystem → outside
// 
// No payload carries FTerminalActivityRecord entirely.
// The full game snapshot (including the entire TArray<FTerminalStageProgress>)
// is read via UTerminalSubsystem::GetGameRecord(...).
// 
// ИСХОДЯЩИЕ нотификации: подсистема → наружу
//
// Ни один payload не тащит FTerminalActivityRecord целиком.
// Полный снимок партии (включая весь TArray<FTerminalStageProgress>)
// читается через UTerminalSubsystem::GetGameRecord(...).
// ============================================================================

// ---- Game started ----
// ---- Игра началась ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameStartedEventPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid     TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString   ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FDateTime StartedAt;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameStartedEventPayload* Setup(const FGuid& InTid, const FString& InAid,
        const FDateTime& InStartedAt)
    {
        TerminalId = InTid; ActivityId = InAid; StartedAt = InStartedAt; return this;
    }
};

// ---- Stage finished (with result: Completed / Failed / Skipped) ----
// ---- Этап завершён (с результатом: Completed / Failed / Skipped) ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameStageFinishedEventPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;

    // One completed stage entirely: StageId, Result, Score, CompletedAt, Notes.
    // Один завершённый этап целиком: StageId, Result, Score, CompletedAt, Notes.
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game")
    FTerminalStageProgress Stage;

    // Accumulated score for the session at the time of the event.
    // Накопленный счёт по партии на момент события.
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game")
    int32 TotalScore = 0;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameStageFinishedEventPayload* Setup(const FGuid& InTid, const FString& InAid,
        const FTerminalStageProgress& InStage, int32 InTotalScore)
    {
        TerminalId = InTid; ActivityId = InAid; Stage = InStage; TotalScore = InTotalScore;
        return this;
    }
};

// ---- Game completed ----
// ---- Игра завершена ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameCompletedEventPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid     TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString   ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") bool      bSuccess = false;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") int32     TotalScore = 0;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString   ResultData;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FDateTime StartedAt;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FDateTime FinishedAt;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameCompletedEventPayload* Setup(const FGuid& InTid, const FString& InAid,
        bool bInSuccess, int32 InTotalScore, const FString& InResultData,
        const FDateTime& InStartedAt, const FDateTime& InFinishedAt)
    {
        TerminalId = InTid; ActivityId = InAid;
        bSuccess = bInSuccess; TotalScore = InTotalScore; ResultData = InResultData;
        StartedAt = InStartedAt; FinishedAt = InFinishedAt;
        return this;
    }
};

// ---- Record removed ----
// ---- Запись удалена ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameRecordRemovedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameRecordRemovedPayload* Setup(const FGuid& InTid, const FString& InAid)
    {
        TerminalId = InTid; ActivityId = InAid; return this;
    }
};

// ---- Removing a single stage of a specific game ----
// ---- Удаление одного этапа конкретной игры ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameStageRemovedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString StageId;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameStageRemovedPayload* Setup(const FGuid& InTid,
        const FString& InAid, const FString& InSid)
    {
        TerminalId = InTid; ActivityId = InAid; StageId = InSid; return this;
    }
};

// ---- Stage removed ----
// ---- Этап удалён ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalGameStageRemovedReportPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString ActivityId;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|Game") FString StageId;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Game")
    UTerminalGameStageRemovedReportPayload* Setup(const FGuid& InTid,
        const FString& InAid, const FString& InSid)
    {
        TerminalId = InTid; ActivityId = InAid; StageId = InSid; return this;
    }
};

// ============================================================================
// TextFilesByTimeBase: outgoing reports
// ИСХОДЯЩИЕ отчёты: подсистема → наружу
//
// Структура: TMap<TerminalID, TMap<Profile, TMap<FileName, FTextBackupTimeStamp>>>
// ============================================================================

// ---- Text file backup added ----
// ---- Резервная копия добавлена / заменена ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalTextFileBackupAddedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString TerminalID;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString Profile;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString FileName;

    // Полная новая структура записи (после мутации).
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile")
    FTextBackupTimeStamp Backup;

    UFUNCTION(BlueprintCallable, Category = "Terminal|TextFile")
    UTerminalTextFileBackupAddedPayload* Setup(
        const FString& InTerminalID,
        const FString& InProfile,
        const FString& InFileName,
        const FTextBackupTimeStamp& InBackup)
    {
        TerminalID = InTerminalID;
        Profile = InProfile;
        FileName = InFileName;
        Backup = InBackup;
        return this;
    }
};

// ---- Text file backup removed ----
// ---- Резервная копия удалена ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalTextFileBackupRemovedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString TerminalID;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString Profile;
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString FileName;
    // Пустой — был удалён весь файл.
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString TimestampKey;

    UFUNCTION(BlueprintCallable, Category = "Terminal|TextFile")
    UTerminalTextFileBackupRemovedPayload* Setup(
        const FString& InTerminalID, const FString& InProfile,
        const FString& InFileName, const FString& InKey)
    {
        TerminalID = InTerminalID;
        Profile = InProfile;
        FileName = InFileName;
        TimestampKey = InKey;
        return this;
    }
};

// ---- Text file backups cleared ----
// ---- Резервные копии очищены ----
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalTextFileBackupsClearedPayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString TerminalID;
    // Пустой — очищен весь Profile.
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString Profile;
    // Пустой — очищен весь TerminalID.
    UPROPERTY(BlueprintReadWrite, Category = "Terminal|TextFile") FString FileName;

    UFUNCTION(BlueprintCallable, Category = "Terminal|TextFile")
    UTerminalTextFileBackupsClearedPayload* Setup(
        const FString& InTerminalID, const FString& InProfile, const FString& InFileName)
    {
        TerminalID = InTerminalID;
        Profile = InProfile;
        FileName = InFileName;
        return this;
    }
};