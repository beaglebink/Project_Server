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
    None                    UMETA(DisplayName = "None"),
    CanReadLocalFiles       UMETA(DisplayName = "Can Read Local Files"),
    CanWriteLocalFiles      UMETA(DisplayName = "Can Write Local Files"),
    CanAccessGlobalEmail    UMETA(DisplayName = "Can Access Global Email"),
    CanAccessSharedSites    UMETA(DisplayName = "Can Access Shared Sites"),
    CanRunMinigames         UMETA(DisplayName = "Can Run Minigames"),
    CanModifyBuildingSecurity UMETA(DisplayName = "Can Modify Building Security"),
    CanStoreCapturedGhosts  UMETA(DisplayName = "Can Store Captured Ghosts"),
    RequiresPassword        UMETA(DisplayName = "Requires Password"),
    RequiresAccountLogin    UMETA(DisplayName = "Requires Account Login"),
    ContextualAccessOnly    UMETA(DisplayName = "Contextual Access Only")
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

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTerminalSubsystem : public UGameInstanceSubsystem, public FInteractiveSubsystemMethods, public ISaveableSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // ------------------------------------------------------------------------
    // Registration (from interactive items)
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Registration")
    void SubscribeRegistration();

    UFUNCTION(BlueprintCallable, Category = "Terminal|Registration")
    void UnsubscribeRegistration();

    // Per-item listener API (from FInteractiveSubsystemMethods)
    void AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);
    void RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener);

    // ------------------------------------------------------------------------
    // Terminal management
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Management")
    bool RegisterTerminal(const FGuid& TerminalId, AActor* ProfileActor = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Management")
    bool UnregisterTerminal(const FGuid& TerminalId);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Management")
    bool IsTerminalRegistered(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Management")
    FTerminalState GetTerminalState(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Management")
    void SetTerminalCapabilities(const FGuid& TerminalId, const TArray<ETerminalCapability>& NewCapabilities);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Management")
    void ApplyProfileToTerminal(const FGuid& TerminalId, AActor* ProfileActor);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Management")
    void ResetTerminalToDefault(const FGuid& TerminalId);

    // ------------------------------------------------------------------------
    // Access / Authentication
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Access")
    bool OpenTerminal(const FGuid& TerminalId);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Access")
    bool CloseTerminal(const FGuid& TerminalId);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Access")
    bool LoginWithPassword(const FGuid& TerminalId, const FString& Password);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Access")
    bool LoginWithAccount(const FGuid& TerminalId, const FString& AccountName);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Access")
    bool ContextualAccess(const FGuid& TerminalId, const FString& ContextToken);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Access")
    bool IsTerminalAccessible(const FGuid& TerminalId) const;

    // ------------------------------------------------------------------------
    // Local file operations
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Files")
    bool ReadLocalFile(const FGuid& TerminalId, const FString& FileName, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Files")
    bool WriteLocalFile(const FGuid& TerminalId, const FString& FileName, const FString& Content);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Files")
    bool DeleteLocalFile(const FGuid& TerminalId, const FString& FileName);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Files")
    TArray<FString> GetLocalFileNames(const FGuid& TerminalId) const;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Files")
    TArray<FTerminalFileEntry> GetFilesInDirectory(const FGuid& TerminalId, const FString& DirectoryPath) const;

    // ------------------------------------------------------------------------
    // Global services (email, websites, etc.)
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Global")
    bool GetEmailContent(const FString& Account, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Global")
    bool SetEmailContent(const FString& Account, const FString& Content);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Global")
    bool GetWebsiteContent(const FString& Url, FString& OutContent) const;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Global")
    bool SetWebsiteContent(const FString& Url, const FString& Content);

    UFUNCTION(BlueprintCallable, Category = "Terminal|Global")
    bool GetGlobalData(const FString& Key, FString& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Terminal|Global")
    bool SetGlobalData(const FString& Key, const FString& Value);

    // ------------------------------------------------------------------------
    // Logging
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Logs")
    void AddLocalLogEntry(const FGuid& TerminalId, const FString& Message);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Terminal|Logs")
    TArray<FTerminalLogEntry> GetLocalLogs(const FGuid& TerminalId) const;

    // ------------------------------------------------------------------------
    // Event publishing helpers
    // ------------------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Terminal|Events")
    void PublishTerminalOutcome(const FGuid& TerminalId, EOutcomeTerminal OutcomeType, UOutcomePayload* Payload = nullptr);

    // ------------------------------------------------------------------------
    // Delegates
    // ------------------------------------------------------------------------
    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalEvent OnTerminalEvent;

    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalStateChanged OnTerminalStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "Terminal|Events")
    FOnTerminalGlobalStateChanged OnTerminalGlobalStateChanged;

    // ------------------------------------------------------------------------
    // ISaveableSubsystem interface
    // ------------------------------------------------------------------------
    virtual void CollectSaveData(FSubsystemSaveData& OutData) override;
    virtual void ApplySaveData(const FSubsystemSaveData& InData) override;
    virtual FString GetSaveSubsystemName() const override { return TEXT("TerminalSubsystem"); }
    virtual bool GetIsLoadComplete() const override { return bIsLoadComplete; }


    // Default profile actor (Blueprint instance) – set in editor or at runtime
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terminal|Default")
    TObjectPtr<AActor> DefaultProfileActor;

private:
    // ------------------------------------------------------------------------
    // EventBus handlers
    // ------------------------------------------------------------------------
    void HandleInteractRegistration(const FOutcomeEventBase& Outcome);
    void HandleInteractCommand(const FOutcomeEventBase& Outcome);
    void HandleSetEnabled(const FOutcomeEventBase& Outcome);
    void HandleSetRange(const FOutcomeEventBase& Outcome);
    void HandleSetTooltip(const FOutcomeEventBase& Outcome);
    void HandleTestInteractCommand(const FOutcomeEventBase& Outcome);

    // ------------------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------------------
    bool IsTerminalCapable(const FGuid& TerminalId, ETerminalCapability Capability) const;
    void UpdateTerminalState(const FGuid& TerminalId, const FTerminalState& NewState);
    void BroadcastTerminalState(const FGuid& TerminalId);
    void BroadcastGlobalState();

    // Initialize terminal state from a profile actor (implements ITerminalProfileProvider)
    void InitializeTerminalFromProfile(FTerminalState& State, AActor* ProfileActor);
    // Helper to get interface from actor
    static ITerminalProfileProvider* GetProfileInterface(AActor* Actor);

    // ------------------------------------------------------------------------
    // Subscriptions
    // ------------------------------------------------------------------------
    void SubscribeAll();
    void UnsubscribeAll();

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

    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
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

    // Handles
    FOutcomeHandlerHandle RegisteredRegisterHandle;
    FOutcomeHandlerHandle UnregisteredRegisterHandle;
    FOutcomeHandlerHandle InteractCommandHandle;
    FOutcomeHandlerHandle SetEnabledHandle;
    FOutcomeHandlerHandle SetRangeHandle;
    FOutcomeHandlerHandle SetTooltipHandle;
    FOutcomeHandlerHandle TerminalCommandHandle;

    // Condition assets
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

    bool bIsLoadComplete = true;

    // Implementation of FInteractiveSubsystemMethods
    virtual TMap<FGuid, TArray<TWeakObjectPtr<UInteractiveItemComponent>>>& GetRegistrationListeners() override
    {
        return RegistrationListeners;
    }
};