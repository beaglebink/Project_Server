#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OutcomeEventBase.h"
#include "OutcomeConditionAsset.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractiveSubsystemMethods.h"
#include "ISaveableSubsystem.h"
#include <FloorAssignmentComponent.h>
#include "TerminalSubsystem.generated.h"

class UInteractiveItemComponent;
class UBookfaceSubsystem;
class UInstantMessengerSubsystem;
class ITerminalProfileProvider;

// ----------------------------------------------------------------------------
// Data structures (unchanged)
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
    UPROPERTY(BlueprintReadWrite)
    FString FileName;
    UPROPERTY(BlueprintReadWrite)
    FString Content;
    UPROPERTY(BlueprintReadWrite)
    FDateTime LastModified;
    UPROPERTY(BlueprintReadWrite)
    FString ParentPath; // empty = root
};

USTRUCT(BlueprintType)
struct FTerminalLogEntry
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite)
    FDateTime Timestamp;
    UPROPERTY(BlueprintReadWrite)
    FString Message;
};

USTRUCT(BlueprintType)
struct FTerminalState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite)
    FGuid TerminalId;
    UPROPERTY(BlueprintReadWrite)
    bool bIsLocked = true;
    UPROPERTY(BlueprintReadWrite)
    FString PasswordHash;
    UPROPERTY(BlueprintReadWrite)
    bool bIsAccountLoggedIn = false;
    UPROPERTY(BlueprintReadWrite)
    FString CurrentAccountName;
    UPROPERTY(BlueprintReadWrite)
    TArray<ETerminalCapability> Capabilities;
    UPROPERTY(BlueprintReadWrite)
    TArray<FTerminalFileEntry> LocalFiles;
    UPROPERTY(BlueprintReadWrite)
    TArray<FTerminalLogEntry> LocalLogs;
    UPROPERTY(BlueprintReadWrite)
    bool bIsOpen = false;
    UPROPERTY(BlueprintReadWrite)
    FString ProfileName; // for debug/info
};

USTRUCT(BlueprintType)
struct FTerminalGlobalState
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> EmailBoxes;
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> Websites;
    UPROPERTY(BlueprintReadWrite)
    TMap<FString, FString> SharedData;
};

// ----------------------------------------------------------------------------
// Internal record for registered interactive items
// ----------------------------------------------------------------------------

USTRUCT()
struct FTerminalInteractRecord
{
    GENERATED_BODY()

    UPROPERTY()
    FGuid ItemId;

    UPROPERTY()
    TWeakObjectPtr<AActor> OwnerActor;

    UPROPERTY()
    float InteractionRange = 0.f;
};

// ----------------------------------------------------------------------------
// Delegates
// ----------------------------------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalEvent, const FOutcomeEventBase&, Outcome);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerminalStateChanged, const FGuid&, TerminalId, const FTerminalState&, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerminalGlobalStateChanged, const FTerminalGlobalState&, NewGlobalState);

// ----------------------------------------------------------------------------
// Subsystem class
// ----------------------------------------------------------------------------
//
// Публичный API подсистемы — только ГЕТТЕРЫ. Все мутирующие операции
// выполняются через обработчики команд, приходящих из EventBus (стиль
// ChoreManagerSubsystem). Команды публикуются вызывающей стороной через
// PublishOutcome с соответствующим EOutcomeTerminal::*Request и payload'ом
// из TerminalCommandPayloads.h.
//
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

    // ------------------------------------------------------------------------
    // ISaveableSubsystem interface
    // ------------------------------------------------------------------------
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("TerminalSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }

    // ------------------------------------------------------------------------
    // Per-item listener API (from FInteractiveSubsystemMethods)
    // ------------------------------------------------------------------------
    void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
    void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

    // ========================================================================
    // Публичные ГЕТТЕРЫ (не меняют состояние подсистемы)
    // ========================================================================

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query")
    bool IsTerminalRegistered(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query")
    FTerminalState GetTerminalState(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query")
    bool IsTerminalAccessible(const FGuid& TerminalId) const;

    // ------------------------------------------------------------------------
    // Local files (read-only)
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Files")
    bool ReadLocalFile(const FGuid& TerminalId, const FString& FileName, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Files")
    TArray<FString> GetLocalFileNames(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Files")
    TArray<FTerminalFileEntry> GetFilesInDirectory(const FGuid& TerminalId, const FString& DirectoryPath) const;

    // ------------------------------------------------------------------------
    // Global services (read-only)
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Global")
    bool GetEmailContent(const FString& Account, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Global")
    bool GetWebsiteContent(const FString& Url, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Global")
    bool GetGlobalData(const FString& Key, FString& OutValue) const;

    // ------------------------------------------------------------------------
    // Logs (read-only)
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Query|Logs")
    TArray<FTerminalLogEntry> GetLocalLogs(const FGuid& TerminalId) const;

    // ------------------------------------------------------------------------
    // Delegates (нотификации наружу)
    // ------------------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalEvent OnTerminalEvent;

    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalStateChanged OnTerminalStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalGlobalStateChanged OnTerminalGlobalStateChanged;

    // Default profile actor (Blueprint instance) – set in editor or at runtime
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Default")
    TObjectPtr<AActor> DefaultProfileActor;

protected:
    // ========================================================================
    // МУТАТОРЫ — доступны только внутри подсистемы и её обработчиков.
    // Внешний код должен публиковать соответствующую команду в EventBus
    // (см. TerminalCommandPayloads.h и EOutcomeTerminal::*Request).
    // ========================================================================

    // ---- Registration ----
    bool RegisterTerminal(const FGuid& TerminalId, AActor* ProfileActor = nullptr);
    bool UnregisterTerminal(const FGuid& TerminalId);
    void SetTerminalCapabilities(const FGuid& TerminalId, const TArray<ETerminalCapability>& NewCapabilities);
    void ApplyProfileToTerminal(const FGuid& TerminalId, AActor* ProfileActor);
    void ResetTerminalToDefault(const FGuid& TerminalId);

    // ---- Access / Authentication ----
    bool OpenTerminal(const FGuid& TerminalId);
    bool CloseTerminal(const FGuid& TerminalId);
    bool LoginWithPassword(const FGuid& TerminalId, const FString& Password);
    bool LoginWithAccount(const FGuid& TerminalId, const FString& AccountName);
    bool ContextualAccess(const FGuid& TerminalId, const FString& ContextToken);

    // ---- Local file operations ----
    bool WriteLocalFile(const FGuid& TerminalId, const FString& FileName, const FString& Content);
    bool DeleteLocalFile(const FGuid& TerminalId, const FString& FileName);

    // ---- Global services ----
    bool SetEmailContent(const FString& Account, const FString& Content);
    bool SetWebsiteContent(const FString& Url, const FString& Content);
    bool SetGlobalData(const FString& Key, const FString& Value);

    // ---- Logging ----
    void AddLocalLogEntry(const FGuid& TerminalId, const FString& Message);

    // ---- Event publishing helper ----
    void PublishTerminalOutcome(const FGuid& TerminalId, EOutcomeTerminal OutcomeType, UOutcomePayload* Payload = nullptr);

private:
    // ========================================================================
    // Обработчики команд из EventBus
    // ========================================================================

    // ---- Lifecycle ----
    void HandleRegisterTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleUnregisterTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleSetCapabilitiesRequest(const FOutcomeEventBase& Outcome);
    void HandleApplyProfileRequest(const FOutcomeEventBase& Outcome);
    void HandleResetToDefaultRequest(const FOutcomeEventBase& Outcome);

    // ---- Access / Authentication ----
    void HandleOpenTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleCloseTerminalRequest(const FOutcomeEventBase& Outcome);
    void HandleLoginPasswordRequest(const FOutcomeEventBase& Outcome);
    void HandleLoginAccountRequest(const FOutcomeEventBase& Outcome);
    void HandleContextualAccessRequest(const FOutcomeEventBase& Outcome);

    // ---- Files ----
    void HandleWriteFileRequest(const FOutcomeEventBase& Outcome);
    void HandleDeleteFileRequest(const FOutcomeEventBase& Outcome);

    // ---- Global services ----
    void HandleSetEmailRequest(const FOutcomeEventBase& Outcome);
    void HandleSetWebsiteRequest(const FOutcomeEventBase& Outcome);
    void HandleSetGlobalDataRequest(const FOutcomeEventBase& Outcome);

    // ---- Logging ----
    void HandleAddLogRequest(const FOutcomeEventBase& Outcome);

    // ---- Interact handlers (existing) ----
    void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
    void HandleInteractCommand(const FOutcomeEventBase& Outcome);
    void HandleSetEnabled(const FOutcomeEventBase& Outcome);
    void HandleSetRange(const FOutcomeEventBase& Outcome);
    void HandleSetTooltip(const FOutcomeEventBase& Outcome);
    void HandleTestInteractCommand(const FOutcomeEventBase& Outcome);

    // ========================================================================
    // Internal helpers
    // ========================================================================
    UOutcomeConditionAsset* CreateSimpleTerminalCondition(EOutcomeTerminal TerminalType);

    bool IsTerminalCapable(const FGuid& TerminalId, ETerminalCapability Capability) const;
    void UpdateTerminalState(const FGuid& TerminalId, const FTerminalState& NewState);
    void BroadcastTerminalState(const FGuid& TerminalId);
    void BroadcastGlobalState();

    // Initialize terminal state from a profile actor (implements ITerminalProfileProvider)
    void InitializeTerminalFromProfile(FTerminalState& State, AActor* ProfileActor);
    // Helper to get interface from actor
    static ITerminalProfileProvider* GetProfileInterface(AActor* Actor);

    // ========================================================================
    // Subscriptions
    // ========================================================================
    void SubscribeAll();
    void UnsubscribeAll();

    void SubscribeRegistration();
    void UnsubscribeRegistration();

    void SubscribeInteractCommand();
    void UnsubscribeInteractCommand();

    void SubscribeSetEnabled();
    void UnsubscribeSetEnabled();

    void SubscribeSetRange();
    void UnsubscribeSetRange();

    void SubscribeSetTooltip();
    void UnsubscribeSetTooltip();

    void SubscribeTestInteractCommand();
    void UnsubscribeTestInteractCommand();

    // ---- Command subscriptions (new) ----
    void SubscribeCommands();
    void UnsubscribeCommands();

    // ========================================================================
    // Data
    // ========================================================================
    UPROPERTY()
    TMap<FGuid, FTerminalState> Terminals;

    UPROPERTY()
    FTerminalGlobalState GlobalState;

    UPROPERTY()
    TMap<FGuid, FTerminalInteractRecord> RegisteredItems;

    // Per-item registration listeners
    TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>> RegistrationListeners;

    // Cached subsystems
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

    // ---- Command handles (new) ----
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

    // ---- Interact condition assets ----
    UPROPERTY()
    UOutcomeConditionAsset* RegisteredConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* UnregisteredConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* InteractCommandConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* SetEnabledConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* SetRangeConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* SetTooltipConditionAsset = nullptr;
    UPROPERTY()
    UOutcomeConditionAsset* TerminalCommandConditionAsset = nullptr;

    // ---- Command condition assets (new) ----
    UPROPERTY() 
    UOutcomeConditionAsset* RegisterTerminalCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* UnregisterTerminalCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* SetCapabilitiesCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* ApplyProfileCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* ResetToDefaultCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* OpenTerminalCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* CloseTerminalCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* LoginPasswordCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* LoginAccountCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* ContextualAccessCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* WriteFileCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* DeleteFileCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* SetEmailCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* SetWebsiteCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* SetGlobalDataCondition = nullptr;
    UPROPERTY() 
    UOutcomeConditionAsset* AddLogCondition = nullptr;
    bool bIsLoadComplete = true;

    // Implementation of FInteractiveSubsystemMethods
    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }
};