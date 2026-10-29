#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "ISaveableSubsystem.h"
#include <FloorAssignmentComponent.h>
#include "FPSKitALSRefactored/CoreGameplay/Cubixon/Core/CubixonCoreData.h"
#include "TerminalSubsystem.generated.h"

class UInteractiveItemComponent;
class UBookfaceSubsystem;
class UInstantMessengerSubsystem;
class ITerminalProfileProvider;

// ----------------------------------------------------------------------------
// Data structures
// ----------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ETerminalCapability : uint8
{
    None                        UMETA(DisplayName = "None"),
    CanReadLocalFiles           UMETA(DisplayName = "Can Read Local Files"),
    CanWriteLocalFiles          UMETA(DisplayName = "Can Write Local Files"),
    CanAccessGlobalEmail        UMETA(DisplayName = "Can Access Global Email"),
    CanAccessSharedSites        UMETA(DisplayName = "Can Access Shared Sites"),
    CanRunMinigames             UMETA(DisplayName = "Can Run Minigames"),
    CanModifyBuildingSecurity   UMETA(DisplayName = "Can Modify Building Security"),
    CanStoreCapturedGhosts      UMETA(DisplayName = "Can Store Captured Ghosts"),
    RequiresPassword            UMETA(DisplayName = "Requires Password"),
    RequiresAccountLogin        UMETA(DisplayName = "Requires Account Login"),
    ContextualAccessOnly        UMETA(DisplayName = "Contextual Access Only")
};

USTRUCT(BlueprintType)
struct FTerminalFileEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FString FileName;
    UPROPERTY(BlueprintReadWrite) FString Content;
    UPROPERTY(BlueprintReadWrite) FDateTime LastModified;
    // empty = root
    // пустой = корень
    UPROPERTY(BlueprintReadWrite) FString ParentPath;
};

USTRUCT(BlueprintType)
struct FTerminalLogEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FDateTime Timestamp;
    UPROPERTY(BlueprintReadWrite) FString Message;
};

USTRUCT(BlueprintType)
struct FTerminalState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) FGuid TerminalId;
    UPROPERTY(BlueprintReadWrite) bool bIsLocked = true;
    UPROPERTY(BlueprintReadWrite) FString PasswordHash;
    UPROPERTY(BlueprintReadWrite) bool bIsAccountLoggedIn = false;
    UPROPERTY(BlueprintReadWrite) FString CurrentAccountName;
    UPROPERTY(BlueprintReadWrite) TArray<ETerminalCapability> Capabilities;
    UPROPERTY(BlueprintReadWrite) TArray<FTerminalFileEntry> LocalFiles;
    UPROPERTY(BlueprintReadWrite) TArray<FTerminalLogEntry> LocalLogs;
    UPROPERTY(BlueprintReadWrite) bool bIsOpen = false;
    UPROPERTY(BlueprintReadWrite) FString ProfileName;
};

USTRUCT(BlueprintType)
struct FTerminalGlobalState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite) TMap<FString, FString> EmailBoxes;
    UPROPERTY(BlueprintReadWrite) TMap<FString, FString> Websites;
    UPROPERTY(BlueprintReadWrite) TMap<FString, FString> SharedData;
};

// ----------------------------------------------------------------------------
// Records of completed games and their stages
// Записи о пройденных играх и их этапах
// ----------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ETerminalRecordStatus : uint8
{
    NotStarted      UMETA(DisplayName = "Not Started"),
    Started         UMETA(DisplayName = "Started"),
    InProgress      UMETA(DisplayName = "In Progress"),
    GameCompleted   UMETA(DisplayName = "Game Completed"),
    Failed          UMETA(DisplayName = "Failed")
};

// Result of an individual stage.
// Результат отдельного этапа.
UENUM(BlueprintType)
enum class ETerminalStageResult : uint8
{
    None        UMETA(DisplayName = "None", Hidden),
    Completed   UMETA(DisplayName = "Completed"),
    Failed      UMETA(DisplayName = "Failed"),
    Skipped     UMETA(DisplayName = "Skipped")
};

USTRUCT(BlueprintType)
struct FTerminalStageProgress
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FString   StageId;

    // How the stage ended. None — has not been finished yet.
    // Чем закончился этап. None — ещё не завершался.
    UPROPERTY(BlueprintReadWrite) ETerminalStageResult Result = ETerminalStageResult::None;

    UPROPERTY(BlueprintReadWrite) int32     Score = 0;

    // The moment the stage ended (success, failure, or skip).
    // Момент окончания этапа (успех, провал или пропуск).
    UPROPERTY(BlueprintReadWrite) FDateTime CompletedAt;

    UPROPERTY(BlueprintReadWrite) FString   Notes;

    // A regular C++ method (UFUNCTION inside USTRUCT is forbidden by UHT).
    // Обычный C++-метод (UFUNCTION внутри USTRUCT запрещён UHT).
    bool IsFinished() const { return Result != ETerminalStageResult::None; }
};

USTRUCT(BlueprintType)
struct FTerminalActivityRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite) FGuid   TerminalId;
    UPROPERTY(BlueprintReadWrite) FString ActivityId;   // GameId
    UPROPERTY(BlueprintReadWrite) ETerminalRecordStatus Status = ETerminalRecordStatus::NotStarted;

    UPROPERTY(BlueprintReadWrite) TArray<FTerminalStageProgress> Stages;
    UPROPERTY(BlueprintReadWrite) int32   TotalScore = 0;

    // Start of the current session.
    // Начало текущей партии.
    UPROPERTY(BlueprintReadWrite) FDateTime StartedAt;

    // End of the game. Remains zero while the game is running.
    // Окончание игры. Остаётся нулевым, пока игра идёт.
    UPROPERTY(BlueprintReadWrite) FDateTime FinishedAt;

    UPROPERTY(BlueprintReadWrite) FString ResultData;

    bool IsFinished() const
    {
        return Status == ETerminalRecordStatus::GameCompleted
            || Status == ETerminalRecordStatus::Failed;
    }
};

// ----------------------------------------------------------------------------
// Internal record for registered interactive items
// ----------------------------------------------------------------------------

USTRUCT()
struct FTerminalInteractRecord
{
    GENERATED_BODY()
    UPROPERTY() FGuid ItemId;
    UPROPERTY() TWeakObjectPtr<AActor> OwnerActor;
    UPROPERTY() float InteractionRange = 0.f;
};

// ----------------------------------------------------------------------------
// Delegates
// ----------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerminalStateChanged, const FGuid&, TerminalId, const FTerminalState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalGlobalStateChanged, const FTerminalGlobalState&, NewGlobalState);

// ----------------------------------------------------------------------------
// Subsystem
// ----------------------------------------------------------------------------

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalSubsystem
    : public UGameInstanceSubsystem
    , public FInteractiveSubsystemMethods
    , public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ---- ISaveableSubsystem ----
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("TerminalSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

    // ---- Per-item listener API ----
    void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
    void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

    // ========================================================================
    // Public GETTERS
    // Публичные ГЕТТЕРЫ
    // ========================================================================

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query")
    bool IsTerminalRegistered(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query")
    FTerminalState GetTerminalState(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query")
    bool IsTerminalAccessible(const FGuid& TerminalId) const;

    // ---- Files ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Files")
    bool ReadLocalFile(const FGuid& TerminalId, const FString& FileName, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Files")
    TArray<FString> GetLocalFileNames(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Files")
    TArray<FTerminalFileEntry> GetFilesInDirectory(const FGuid& TerminalId, const FString& DirectoryPath) const;

    // ---- Global services ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Global")
    bool GetEmailContent(const FString& Account, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Global")
    bool GetWebsiteContent(const FString& Url, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Global")
    bool GetGlobalData(const FString& Key, FString& OutValue) const;

    // ---- Logs ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Logs")
    TArray<FTerminalLogEntry> GetLocalLogs(const FGuid& TerminalId) const;

    // ---- Games (read-only) ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    bool HasGameRecord(const FGuid& TerminalId, const FString& GameId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    bool GetGameRecord(const FGuid& TerminalId, const FString& GameId,
        FTerminalActivityRecord& OutRecord) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    TArray<FTerminalActivityRecord> GetGameRecordsForTerminal(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    TArray<FTerminalActivityRecord> GetAllGameRecordsAcrossTerminals() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    TArray<FTerminalActivityRecord> GetGameRecordsByStatus(ETerminalRecordStatus Status, const FGuid& TerminalId = FGuid()) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    TArray<FTerminalActivityRecord> GetAllGameRecordsByActivityId(const FString& ActivityId, const FGuid& TerminalId = FGuid()) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    bool HasGameStage(const FGuid& TerminalId, const FString& GameId,
        const FString& StageId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    bool GetGameStage(const FGuid& TerminalId, const FString& GameId,
        const FString& StageId, FTerminalStageProgress& OutStage) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    TArray<FTerminalStageProgress> GetGameStages(const FGuid& TerminalId,
        const FString& GameId) const;

    // ---- Helpers for Blueprint ----
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    static bool IsGameRecordFinished(const FTerminalActivityRecord& Record)
    {
        return Record.IsFinished();
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Games")
    bool IsGameFinishedForTerminal(const FGuid& TerminalId, const FString& GameId) const;

    // ---- Text file backups (TimeBase) — только чтение ----
    // Outer key — TerminalID. Middle key — Profile. Inner key — FileName.

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //TArray<FString> GetAllTextFileBackupTerminalIDs() const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //TArray<FString> GetAllTextFileBackupProfiles(const FString& TerminalID) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //bool HasTextFileBackupTerminal(const FString& TerminalID) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //bool HasTextFileBackupProfile(const FString& TerminalID, const FString& Profile) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //TArray<FString> GetTextFileBackupFileNames(const FString& TerminalID,
    //    const FString& Profile) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //bool HasTextFileBackup(const FString& TerminalID, const FString& Profile,
    //    const FString& FileName) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //bool GetTextFileBackupSize(const FString& TerminalID, const FString& Profile,
    //    const FString& FileName, float& OutSize) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //bool GetTextFileBackupEntry(const FString& TerminalID, const FString& Profile,
    //    const FString& FileName, const FString& TimestampKey, FText& OutContent) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //TArray<FString> GetTextFileBackupKeys(const FString& TerminalID,
    //    const FString& Profile, const FString& FileName) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //bool GetTextFileBackup(const FString& TerminalID, const FString& Profile,
    //    const FString& FileName, FTextBackupTimeStamp& OutData) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //float GetTextFileBackupTotalSize(const FString& TerminalID,
    //    const FString& Profile) const;

    //UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    //float GetTextFileBackupGrandTotalSize() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|TextFiles")
    TMap<FString, FTextBackupTimeStamp> GetTextFilesByTime(const FString& TerminalID,
        const FString& Profile) const;

    // ---- Delegates ----
    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalEvent OnTerminalEvent;

    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalStateChanged OnTerminalStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalGlobalStateChanged OnTerminalGlobalStateChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Default")
    TObjectPtr<AActor> DefaultProfileActor;

protected:
    // ---- Registration ----
    bool RegisterTerminal(const FGuid& TerminalId, AActor* ProfileActor = nullptr);
    bool UnregisterTerminal(const FGuid& TerminalId);
    void SetTerminalCapabilities(const FGuid& TerminalId, const TArray<ETerminalCapability>& NewCapabilities);
    void ApplyProfileToTerminal(const FGuid& TerminalId, AActor* ProfileActor);
    void ResetTerminalToDefault(const FGuid& TerminalId);

    // ---- Access / Auth ----
    bool OpenTerminal(const FGuid& TerminalId);
    bool CloseTerminal(const FGuid& TerminalId);
    bool LoginWithPassword(const FGuid& TerminalId, const FString& Password);
    bool LoginWithAccount(const FGuid& TerminalId, const FString& AccountName);
    bool ContextualAccess(const FGuid& TerminalId, const FString& ContextToken);

    // ---- Files ----
    bool WriteLocalFile(const FGuid& TerminalId, const FString& FileName, const FString& Content);
    bool DeleteLocalFile(const FGuid& TerminalId, const FString& FileName);

    // ---- Global ----
    bool SetEmailContent(const FString& Account, const FString& Content);
    bool SetWebsiteContent(const FString& Url, const FString& Content);
    bool SetGlobalData(const FString& Key, const FString& Value);

    // ---- Logging ----
    void AddLocalLogEntry(const FGuid& TerminalId, const FString& Message);

    // ---- Event publishing ----
    void PublishTerminalOutcome(const FGuid& TerminalId, EOutcomeTerminal OutcomeType, UOutcomePayload* Payload = nullptr);

    // ---- Text file backups: mutators (private, called from handlers) ----
    // Replace-or-insert: полная структура заменяет запись FileName внутри (TerminalID, Profile).
    bool AddTextFileBackupEntry(const FString& TerminalID, const FString& Profile,
        const FString& FileName, const FTextBackupTimeStamp& Backup);

    // Assign: карта FileName для (TerminalID, Profile) заменяется целиком.
    //void ReplaceTextFileBackups(const FString& TerminalID, const FString& Profile,
    //    const TMap<FString, FTextBackupTimeStamp>& NewBackups);

    // Merge: переданные FileName перезаписываются внутри (TerminalID, Profile),
    // остальные FileName в этом (TerminalID, Profile) сохраняются.
    void OverwriteTextFileBackups(const FString& TerminalID, const FString& Profile,
        const TMap<FString, FTextBackupTimeStamp>& Backups);

    //bool RemoveTextFileBackupEntry(const FString& TerminalID, const FString& Profile,
    //    const FString& FileName, const FString& TimestampKey);

    // Empty Profile  → очистить весь TerminalID (все профили).
    // Empty FileName → очистить весь Profile.
    //bool ClearTextFileBackupEntries(const FString& TerminalID, const FString& Profile,
    //    const FString& FileName);

private:
    // ---- Interact handlers ----
    void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
    void HandleInteractCommand(const FOutcomeEventBase& Outcome);
    //void HandleSetEnabled(const FOutcomeEventBase& Outcome);
    void HandleSetRange(const FOutcomeEventBase& Outcome);
    void HandleSetTooltip(const FOutcomeEventBase& Outcome);
    void HandleTestInteractCommand(const FOutcomeEventBase& Outcome);

    // ---- Lifecycle ----
    void HandleRegisterTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleUnregisterTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleSetCapabilitiesRequest(const FOutcomeEventBase& Outcome);
    void HandleApplyProfileRequest(const FOutcomeEventBase& Outcome);
    void HandleResetToDefaultRequest(const FOutcomeEventBase& Outcome);

    // ---- Access ----
    void HandleOpenTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleCloseTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleLoginPasswordRequest(const FOutcomeEventBase& Outcome);
    void HandleLoginAccountRequest(const FOutcomeEventBase& Outcome);
    void HandleContextualAccessRequest(const FOutcomeEventBase& Outcome);

    // ---- Files ----
    void HandleWriteFileRequest(const FOutcomeEventBase& Outcome);
    void HandleDeleteFileRequest(const FOutcomeEventBase& Outcome);

    // ---- Global ----
    void HandleSetEmailRequest(const FOutcomeEventBase& Outcome);
    void HandleSetWebsiteRequest(const FOutcomeEventBase& Outcome);
    void HandleSetGlobalDataRequest(const FOutcomeEventBase& Outcome);

    // ---- Logging ----
    void HandleAddLogRequest(const FOutcomeEventBase& Outcome);

    // ========================================================================
    // Game command handlers (INCOMING commands from terminals)
    // ========================================================================
    void HandleGameStarted(const FOutcomeEventBase& Outcome);
    void HandleGameStageFinished(const FOutcomeEventBase& Outcome);
    void HandleGameCompleted(const FOutcomeEventBase& Outcome);
    void HandleGameRecordRemoved(const FOutcomeEventBase& Outcome);
    void HandleGameStageRemoved(const FOutcomeEventBase& Outcome);

    // ---- Text file backups: handlers ----
    void HandleAddTextFileBackup(const FOutcomeEventBase& Outcome);
    //void HandleReplaceTextFileBackups(const FOutcomeEventBase& Outcome);
    void HandleOverwriteTextFileBackups(const FOutcomeEventBase& Outcome);
    //void HandleRemoveTextFileBackup(const FOutcomeEventBase& Outcome);
    //void HandleClearTextFileBackups(const FOutcomeEventBase& Outcome);

    // ========================================================================
    // Game mutators (internal)
    // ========================================================================
    void ReportGameStarted(const FGuid& TerminalId, const FString& GameId, const FDateTime& InStartedAt);
    void ReportGameStageFinished(const FGuid& TerminalId, const FString& GameId,
        const FString& StageId, ETerminalStageResult Result, int32 Score, const FString& Notes);
    void ReportGameCompleted(const FGuid& TerminalId, const FString& GameId,
        bool bSuccess, int32 TotalScore, const FString& ResultData);
    void RemoveGameStage(const FGuid& TerminalId, const FString& GameId, const FString& StageId);
    void RemoveGameRecord(const FGuid& TerminalId, const FString& GameId);

    // ---- Helpers ----
    UOutcomeConditionAsset* CreateSimpleTerminalCondition(EOutcomeTerminal TerminalType);
    bool IsTerminalCapable(const FGuid& TerminalId, ETerminalCapability Capability) const;
    void UpdateTerminalState(const FGuid& TerminalId, const FTerminalState& NewState);
    void BroadcastTerminalState(const FGuid& TerminalId);
    void BroadcastGlobalState();

    void InitializeTerminalFromProfile(FTerminalState& State, AActor* ProfileActor);
    static ITerminalProfileProvider* GetProfileInterface(AActor* Actor);

    // ---- Subscriptions ----
    void SubscribeAll();
    void UnsubscribeAll();
    void SubscribeRegistration();
    void UnsubscribeRegistration();
    void SubscribeInteractCommand();
    void UnsubscribeInteractCommand();
    //void SubscribeSetEnabled();
    //void UnsubscribeSetEnabled();
    void SubscribeSetRange();
    void UnsubscribeSetRange();
    void SubscribeSetTooltip();
    void UnsubscribeSetTooltip();
    void SubscribeTestInteractCommand();
    void UnsubscribeTestInteractCommand();
    void SubscribeCommands();
    void UnsubscribeCommands();

    // ========================================================================
    // Data
    // ========================================================================
    UPROPERTY() TMap<FGuid, FTerminalState> Terminals;
    UPROPERTY() FTerminalGlobalState GlobalState;
    UPROPERTY() TMap<FGuid, FTerminalInteractRecord> RegisteredItems;

    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    TWeakObjectPtr<UEventBusSubsystem> CachedEventBus;
    TWeakObjectPtr<UBookfaceSubsystem> CachedBookface;
    TWeakObjectPtr<UInstantMessengerSubsystem> CachedMessenger;

    // ---- Interact handles ----
    FOutcomeHandlerHandle RegisteredRegisterHandle;
    FOutcomeHandlerHandle UnregisteredRegisterHandle;
    FOutcomeHandlerHandle InteractCommandHandle;
    FOutcomeHandlerHandle SetEnabledHandle;
    FOutcomeHandlerHandle SetRangeHandle;
    FOutcomeHandlerHandle SetTooltipHandle;
    FOutcomeHandlerHandle TerminalCommandHandle;

    // ---- Command handles ----
    FOutcomeHandlerHandle RegisterTerminalHandler;
    FOutcomeHandlerHandle UnregisterTerminalHandler;
    FOutcomeHandlerHandle SetCapabilitiesHandler;
    FOutcomeHandlerHandle ApplyProfileHandler;
    FOutcomeHandlerHandle ResetToDefaultHandler;
    FOutcomeHandlerHandle OpenTerminalHandler;
    FOutcomeHandlerHandle CloseTerminalHandler;
    FOutcomeHandlerHandle LoginPasswordHandler;
    FOutcomeHandlerHandle LoginAccountHandler;
    FOutcomeHandlerHandle ContextualAccessHandler;
    FOutcomeHandlerHandle WriteFileHandler;
    FOutcomeHandlerHandle DeleteFileHandler;
    FOutcomeHandlerHandle SetEmailHandler;
    FOutcomeHandlerHandle SetWebsiteHandler;
    FOutcomeHandlerHandle SetGlobalDataHandler;
    FOutcomeHandlerHandle AddLogHandler;

    // ---- Game command handles (incoming) ----
    FOutcomeHandlerHandle GameStartedHandler;
    FOutcomeHandlerHandle GameStageFinishedHandler;
    FOutcomeHandlerHandle GameCompletedHandler;
    FOutcomeHandlerHandle GameStageRemovedHandler;
    FOutcomeHandlerHandle GameRecordRemovedHandler;

    // ---- Text file command handles ----
    FOutcomeHandlerHandle AddTextFileBackupHandler;
    //FOutcomeHandlerHandle ReplaceTextFileBackupsHandler;
    FOutcomeHandlerHandle OverwriteTextFileBackupsHandler;
    //FOutcomeHandlerHandle RemoveTextFileBackupHandler;
    //FOutcomeHandlerHandle ClearTextFileBackupsHandler;

    // ---- Interact conditions ----
    UPROPERTY() UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetEnabledConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;
    UPROPERTY() UOutcomeConditionAsset* TerminalCommandConditionAsset = nullptr;

    // ---- Command conditions ----
    UPROPERTY() UOutcomeConditionAsset* RegisterTerminalCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* UnregisterTerminalCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetCapabilitiesCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* ApplyProfileCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* ResetToDefaultCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* OpenTerminalCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* CloseTerminalCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* LoginPasswordCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* LoginAccountCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* ContextualAccessCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* WriteFileCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* DeleteFileCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetEmailCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetWebsiteCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* SetGlobalDataCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* AddLogCondition = nullptr;

    // ---- Game command conditions (incoming) ----
    UPROPERTY() UOutcomeConditionAsset* GameStartedCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* GameStageFinishedCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* GameCompletedCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* GameStageRemovedCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* GameRecordRemovedCondition = nullptr;

    // ---- Text file command conditions ----
    UPROPERTY() UOutcomeConditionAsset* AddTextFileBackupCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* ReplaceTextFileBackupsCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* OverwriteTextFileBackupsCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* RemoveTextFileBackupCondition = nullptr;
    UPROPERTY() UOutcomeConditionAsset* ClearTextFileBackupsCondition = nullptr;

    bool bIsLoadComplete = true;

    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }

    // Outer key — TerminalId, inner key — GameId.
    // Внешний ключ — TerminalId, внутренний — GameId.
    TMap<FGuid, TMap<FString, FTerminalActivityRecord>> GameRecords;

    // Outer key — TerminalID. Middle key — Profile. Inner key — FileName.
    // Без UPROPERTY: 3-уровневый TMap не поддерживается UHT как отражённый тип.
    // Сериализация — ручная (CollectSaveData / ApplySaveData).
    TMap<FString, TMap<FString, TMap<FString, FTextBackupTimeStamp>>> TextFilesByTimeBase;
};