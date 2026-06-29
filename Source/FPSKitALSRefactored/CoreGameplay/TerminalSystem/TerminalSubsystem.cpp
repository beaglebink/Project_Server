#include "TerminalSubsystem.h"
#include "TerminalProfileProvider.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../EventBusSystem/OutcomeConditionAsset.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "BookfaceSubsystem.h"
#include "InstantMessengerSubsystem.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Base64.h"
#include "TerminalCommandPayload.h"

// ============================================================================
// Helper to get interface
// ============================================================================

ITerminalProfileProvider* UTerminalSubsystem::GetProfileInterface(AActor* Actor)
{
    if (!Actor) return nullptr;
    return Cast<ITerminalProfileProvider>(Actor);
}

// ============================================================================
// Initialization / Deinitialization
// ============================================================================

void UTerminalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CachedEventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    CachedBookface = GetGameInstance()->GetSubsystem<UBookfaceSubsystem>();
    CachedMessenger = GetGameInstance()->GetSubsystem<UInstantMessengerSubsystem>();

    SubscribeAll();

    UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem initialized"));
}

void UTerminalSubsystem::Deinitialize()
{
    UnsubscribeAll();
    CachedEventBus.Reset();
    CachedBookface.Reset();
    CachedMessenger.Reset();
    Super::Deinitialize();
}

// ============================================================================
// Registration (from interactive items)
// ============================================================================

void UTerminalSubsystem::SubscribeRegistration()
{
    if (!CachedEventBus.IsValid()) return;
    if (RegisteredRegisterHandle.IsValid() || UnregisteredRegisterHandle.IsValid()) return;

    RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
    RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    RegisteredConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractRegistered;
    RegisteredConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
    RegisteredConditionAsset->CompileCondition();

    UnregisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    UnregisteredConditionAsset->OperatorType = EConditionOperator::Composite;
    UnregisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    UnregisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    UnregisteredConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractUnregistered;
    UnregisteredConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
    UnregisteredConditionAsset->CompileCondition();

    if (RegisteredConditionAsset->GetCondition().IsValid())
    {
        RegisteredRegisterHandle = CachedEventBus->RegisterHandler(
            RegisteredConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleInteractRegistration)
        );
    }
    if (UnregisteredConditionAsset->GetCondition().IsValid())
    {
        UnregisteredRegisterHandle = CachedEventBus->RegisterHandler(
            UnregisteredConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleInteractRegistration)
        );
    }
}

void UTerminalSubsystem::UnsubscribeRegistration()
{
    if (!CachedEventBus.IsValid()) return;
    if (RegisteredRegisterHandle.IsValid())
    {
        CachedEventBus->UnregisterHandler(RegisteredRegisterHandle);
        RegisteredRegisterHandle.Invalidate();
    }
    if (UnregisteredRegisterHandle.IsValid())
    {
        CachedEventBus->UnregisterHandler(UnregisteredRegisterHandle);
        UnregisteredRegisterHandle.Invalidate();
    }
    RegisteredConditionAsset = nullptr;
    UnregisteredConditionAsset = nullptr;
}

void UTerminalSubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
    FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UTerminalSubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
    FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

void UTerminalSubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
    UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload);
    if (!P) return;

    if (Outcome.OutcomeTerminal == EOutcomeTerminal::InteractRegistered)
    {
        FTerminalInteractRecord Rec;
        Rec.ItemId = P->ItemId;
        Rec.OwnerActor = P->GetOwnerActor();
        Rec.InteractionRange = P->InteractionRange;
        RegisteredItems.Add(P->ItemId, Rec);

        if (!Terminals.Contains(P->ItemId))
        {
            RegisterTerminal(P->ItemId, DefaultProfileActor.Get());
        }

        if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
        {
            for (auto& WeakComp : *List)
            {
                if (UInteractiveItemComponent* Comp = WeakComp.Get())
                    Comp->OnRegisteredBySubsystem(P);
            }
            RegistrationListeners.Remove(P->ItemId);
        }
    }
    else if (Outcome.OutcomeTerminal == EOutcomeTerminal::InteractUnregistered)
    {
        if (Terminals.Contains(P->ItemId)) Terminals.Remove(P->ItemId);
        RegisteredItems.Remove(P->ItemId);

        if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(P->ItemId))
        {
            for (auto& WeakComp : *List)
            {
                if (UInteractiveItemComponent* Comp = WeakComp.Get())
                    Comp->OnUnregisteredBySubsystem(P);
            }
            RegistrationListeners.Remove(P->ItemId);
        }
    }
}

// ============================================================================
// Terminal management
// ============================================================================

void UTerminalSubsystem::InitializeTerminalFromProfile(FTerminalState& State, AActor* ProfileActor)
{
    State.LocalFiles.Empty();
    State.LocalLogs.Empty();
    State.bIsOpen = false;
    State.bIsAccountLoggedIn = false;
    State.CurrentAccountName.Empty();
    State.PasswordHash.Empty();
    State.ProfileName = TEXT("Default");

    ITerminalProfileProvider* Provider = GetProfileInterface(ProfileActor);
    if (!Provider)
    {
        State.Capabilities.Empty();
        State.Capabilities.Add(ETerminalCapability::CanReadLocalFiles);
        State.Capabilities.Add(ETerminalCapability::CanWriteLocalFiles);
        State.bIsLocked = false;
        return;
    }

    State.ProfileName = Provider->GetProfileName_Implementation();
    State.Capabilities = Provider->GetTerminalCapabilities_Implementation();
    State.bIsLocked = Provider->IsTerminalLockedByDefault_Implementation();

    FString Password = Provider->GetTerminalPassword_Implementation();
    if (!Password.IsEmpty())
    {
        State.PasswordHash = FBase64::Encode(Password);
    }

    State.LocalFiles = Provider->GetDefaultFiles_Implementation();
    Provider->ApplyGlobalDefaultData_Implementation(GlobalState);
}

bool UTerminalSubsystem::RegisterTerminal(const FGuid& TerminalId, AActor* ProfileActor)
{
    if (Terminals.Contains(TerminalId))
    {
        UE_LOG(LogTemp, Warning, TEXT("TerminalSubsystem: Terminal %s already registered"), *TerminalId.ToString());
        return false;
    }

    FTerminalState NewState;
    NewState.TerminalId = TerminalId;

    AActor* EffectiveProfile = ProfileActor ? ProfileActor : DefaultProfileActor.Get();
    InitializeTerminalFromProfile(NewState, EffectiveProfile);

    Terminals.Add(TerminalId, NewState);
    BroadcastTerminalState(TerminalId);
    UE_LOG(LogTemp, Log, TEXT("TerminalSubsystem: Terminal %s registered (profile=%s)"),
        *TerminalId.ToString(),
        EffectiveProfile ? *EffectiveProfile->GetName() : TEXT("None"));
    return true;
}

bool UTerminalSubsystem::UnregisterTerminal(const FGuid& TerminalId)
{
    if (!Terminals.Contains(TerminalId)) return false;
    Terminals.Remove(TerminalId);
    BroadcastTerminalState(TerminalId);
    return true;
}

bool UTerminalSubsystem::IsTerminalRegistered(const FGuid& TerminalId) const
{
    return Terminals.Contains(TerminalId);
}

FTerminalState UTerminalSubsystem::GetTerminalState(const FGuid& TerminalId) const
{
    if (const FTerminalState* State = Terminals.Find(TerminalId))
        return *State;
    return FTerminalState();
}

void UTerminalSubsystem::SetTerminalCapabilities(const FGuid& TerminalId, const TArray<ETerminalCapability>& NewCapabilities)
{
    if (FTerminalState* State = Terminals.Find(TerminalId))
    {
        State->Capabilities = NewCapabilities;
        BroadcastTerminalState(TerminalId);
    }
}

void UTerminalSubsystem::ApplyProfileToTerminal(const FGuid& TerminalId, AActor* ProfileActor)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return;

    InitializeTerminalFromProfile(*State, ProfileActor);
    BroadcastTerminalState(TerminalId);
}

void UTerminalSubsystem::ResetTerminalToDefault(const FGuid& TerminalId)
{
    ApplyProfileToTerminal(TerminalId, DefaultProfileActor.Get());
}

// ============================================================================
// Access / Authentication
// ============================================================================

bool UTerminalSubsystem::OpenTerminal(const FGuid& TerminalId)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    if (State->bIsLocked && !State->bIsAccountLoggedIn) return false;
    State->bIsOpen = true;
    BroadcastTerminalState(TerminalId);
    PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalOpened);
    return true;
}

bool UTerminalSubsystem::CloseTerminal(const FGuid& TerminalId)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State || !State->bIsOpen) return false;
    State->bIsOpen = false;
    BroadcastTerminalState(TerminalId);
    PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalClosed);
    return true;
}

bool UTerminalSubsystem::LoginWithPassword(const FGuid& TerminalId, const FString& Password)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    FString Hashed = FBase64::Encode(Password);
    if (State->PasswordHash != Hashed)
    {
        AddLocalLogEntry(TerminalId, TEXT("Failed login attempt with password"));
        PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalLoginFailed);
        return false;
    }
    State->bIsAccountLoggedIn = true;
    State->bIsLocked = false;
    BroadcastTerminalState(TerminalId);
    AddLocalLogEntry(TerminalId, TEXT("Login successful (password)"));
    PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalLoginSucceeded);
    return true;
}

bool UTerminalSubsystem::LoginWithAccount(const FGuid& TerminalId, const FString& AccountName)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State || AccountName.IsEmpty()) return false;
    State->bIsAccountLoggedIn = true;
    State->CurrentAccountName = AccountName;
    State->bIsLocked = false;
    BroadcastTerminalState(TerminalId);
    AddLocalLogEntry(TerminalId, FString::Printf(TEXT("Login successful (account: %s)"), *AccountName));
    PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalLoginSucceeded);
    return true;
}

bool UTerminalSubsystem::ContextualAccess(const FGuid& TerminalId, const FString& ContextToken)
{
    if (ContextToken == TEXT("mission_active"))
    {
        FTerminalState* State = Terminals.Find(TerminalId);
        if (State)
        {
            State->bIsLocked = false;
            State->bIsAccountLoggedIn = true;
            BroadcastTerminalState(TerminalId);
            PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalLoginSucceeded);
            return true;
        }
    }
    return false;
}

bool UTerminalSubsystem::IsTerminalAccessible(const FGuid& TerminalId) const
{
    const FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    return State->bIsOpen && (!State->bIsLocked || State->bIsAccountLoggedIn);
}

// ============================================================================
// Local file operations
// ============================================================================

bool UTerminalSubsystem::ReadLocalFile(const FGuid& TerminalId, const FString& FileName, FString& OutContent) const
{
    const FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    if (!IsTerminalCapable(TerminalId, ETerminalCapability::CanReadLocalFiles)) return false;
    if (!IsTerminalAccessible(TerminalId)) return false;

    for (const FTerminalFileEntry& File : State->LocalFiles)
    {
        if (File.FileName == FileName)
        {
            OutContent = File.Content;
            return true;
        }
    }
    return false;
}

bool UTerminalSubsystem::WriteLocalFile(const FGuid& TerminalId, const FString& FileName, const FString& Content)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    if (!IsTerminalCapable(TerminalId, ETerminalCapability::CanWriteLocalFiles)) return false;
    if (!IsTerminalAccessible(TerminalId)) return false;

    for (FTerminalFileEntry& File : State->LocalFiles)
    {
        if (File.FileName == FileName)
        {
            File.Content = Content;
            File.LastModified = FDateTime::Now();
            AddLocalLogEntry(TerminalId, FString::Printf(TEXT("Modified file: %s"), *FileName));
            PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalFileModified);
            return true;
        }
    }

    FTerminalFileEntry NewFile;
    NewFile.FileName = FileName;
    NewFile.Content = Content;
    NewFile.LastModified = FDateTime::Now();
    State->LocalFiles.Add(NewFile);
    AddLocalLogEntry(TerminalId, FString::Printf(TEXT("Created file: %s"), *FileName));
    PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalFileModified);
    return true;
}

bool UTerminalSubsystem::DeleteLocalFile(const FGuid& TerminalId, const FString& FileName)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    if (!IsTerminalCapable(TerminalId, ETerminalCapability::CanWriteLocalFiles)) return false;
    if (!IsTerminalAccessible(TerminalId)) return false;

    int32 Removed = State->LocalFiles.RemoveAll([&](const FTerminalFileEntry& F) { return F.FileName == FileName; });
    if (Removed > 0)
    {
        AddLocalLogEntry(TerminalId, FString::Printf(TEXT("Deleted file: %s"), *FileName));
        PublishTerminalOutcome(TerminalId, EOutcomeTerminal::TerminalFileModified);
        return true;
    }
    return false;
}

TArray<FString> UTerminalSubsystem::GetLocalFileNames(const FGuid& TerminalId) const
{
    const FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return TArray<FString>();
    TArray<FString> Names;
    for (const FTerminalFileEntry& File : State->LocalFiles)
        Names.Add(File.FileName);
    return Names;
}

TArray<FTerminalFileEntry> UTerminalSubsystem::GetFilesInDirectory(const FGuid& TerminalId, const FString& DirectoryPath) const
{
    const FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return TArray<FTerminalFileEntry>();
    TArray<FTerminalFileEntry> Result;
    for (const FTerminalFileEntry& File : State->LocalFiles)
    {
        if (File.ParentPath == DirectoryPath)
            Result.Add(File);
    }
    return Result;
}

// ============================================================================
// Global services
// ============================================================================

bool UTerminalSubsystem::GetEmailContent(const FString& Account, FString& OutContent) const
{
    if (const FString* Content = GlobalState.EmailBoxes.Find(Account))
    {
        OutContent = *Content;
        return true;
    }
    return false;
}

bool UTerminalSubsystem::SetEmailContent(const FString& Account, const FString& Content)
{
    GlobalState.EmailBoxes.Add(Account, Content);
    BroadcastGlobalState();
    PublishTerminalOutcome(FGuid(), EOutcomeTerminal::TerminalGlobalServiceModified);
    return true;
}

bool UTerminalSubsystem::GetWebsiteContent(const FString& Url, FString& OutContent) const
{
    if (const FString* Content = GlobalState.Websites.Find(Url))
    {
        OutContent = *Content;
        return true;
    }
    return false;
}

bool UTerminalSubsystem::SetWebsiteContent(const FString& Url, const FString& Content)
{
    GlobalState.Websites.Add(Url, Content);
    BroadcastGlobalState();
    PublishTerminalOutcome(FGuid(), EOutcomeTerminal::TerminalGlobalServiceModified);
    return true;
}

bool UTerminalSubsystem::GetGlobalData(const FString& Key, FString& OutValue) const
{
    if (const FString* Value = GlobalState.SharedData.Find(Key))
    {
        OutValue = *Value;
        return true;
    }
    return false;
}

bool UTerminalSubsystem::SetGlobalData(const FString& Key, const FString& Value)
{
    GlobalState.SharedData.Add(Key, Value);
    BroadcastGlobalState();
    PublishTerminalOutcome(FGuid(), EOutcomeTerminal::TerminalGlobalServiceModified);
    return true;
}

// ============================================================================
// Logging
// ============================================================================

void UTerminalSubsystem::AddLocalLogEntry(const FGuid& TerminalId, const FString& Message)
{
    FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return;
    FTerminalLogEntry Entry;
    Entry.Timestamp = FDateTime::Now();
    Entry.Message = Message;
    State->LocalLogs.Add(Entry);
    if (State->LocalLogs.Num() > 100)
        State->LocalLogs.RemoveAt(0);
}

TArray<FTerminalLogEntry> UTerminalSubsystem::GetLocalLogs(const FGuid& TerminalId) const
{
    const FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return TArray<FTerminalLogEntry>();
    return State->LocalLogs;
}

// ============================================================================
// Event publishing
// ============================================================================

void UTerminalSubsystem::PublishTerminalOutcome(const FGuid& TerminalId, EOutcomeTerminal OutcomeType, UOutcomePayload* Payload)
{
    if (!CachedEventBus.IsValid()) return;
    FOutcomeEventBase Ev;
    Ev.OutcomeType = EOutcomeType::Terminal;
    Ev.OutcomeTerminal = OutcomeType;
    Ev.Payload = Payload;
    CachedEventBus->PublishOutcome(Ev);
    OnTerminalEvent.Broadcast(Ev);
}

// ============================================================================
// EventBus handlers (InteractCommand, SetEnabled, SetRange, SetTooltip)
// ============================================================================

void UTerminalSubsystem::SubscribeInteractCommand()
{
    if (!CachedEventBus.IsValid() || InteractCommandHandle.IsValid()) return;
    InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
    InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    InteractCommandConditionAsset->CompileCondition();
    if (InteractCommandConditionAsset->GetCondition().IsValid())
    {
        InteractCommandHandle = CachedEventBus->RegisterHandler(
            InteractCommandConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleInteractCommand)
        );
    }
}

void UTerminalSubsystem::UnsubscribeInteractCommand()
{
    if (!CachedEventBus.IsValid() || !InteractCommandHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(InteractCommandHandle);
    InteractCommandHandle.Invalidate();
    InteractCommandConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
    const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload);
    if (!P) return;
    if (const FTerminalInteractRecord* Rec = RegisteredItems.Find(P->ItemId))
    {
        AActor* Owner = Rec->OwnerActor.Get();
        if (Owner)
        {
            ExecuteInteractCommandOnOwner(P->ItemId, Owner, P->Picker);
        }
    }
}

void UTerminalSubsystem::SubscribeSetEnabled()
{
    if (!CachedEventBus.IsValid() || SetEnabledHandle.IsValid()) return;
    SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
    SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SetEnabledConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractSetEnabled;
    SetEnabledConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
    SetEnabledConditionAsset->CompileCondition();
    if (SetEnabledConditionAsset->GetCondition().IsValid())
    {
        SetEnabledHandle = CachedEventBus->RegisterHandler(
            SetEnabledConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleSetEnabled)
        );
    }
}

void UTerminalSubsystem::UnsubscribeSetEnabled()
{
    if (!CachedEventBus.IsValid() || !SetEnabledHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(SetEnabledHandle);
    SetEnabledHandle.Invalidate();
    SetEnabledConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
    const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload);
    if (!P) return;
    if (const FTerminalInteractRecord* Rec = RegisteredItems.Find(P->ItemId))
        ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
}

void UTerminalSubsystem::SubscribeSetRange()
{
    if (!CachedEventBus.IsValid() || SetRangeHandle.IsValid()) return;
    SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
    SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SetRangeConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractSetRange;
    SetRangeConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
    SetRangeConditionAsset->CompileCondition();
    if (SetRangeConditionAsset->GetCondition().IsValid())
    {
        SetRangeHandle = CachedEventBus->RegisterHandler(
            SetRangeConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleSetRange)
        );
    }
}

void UTerminalSubsystem::UnsubscribeSetRange()
{
    if (!CachedEventBus.IsValid() || !SetRangeHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(SetRangeHandle);
    SetRangeHandle.Invalidate();
    SetRangeConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
    const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload);
    if (!P) return;
    if (const FTerminalInteractRecord* Rec = RegisteredItems.Find(P->ItemId))
        ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
}

void UTerminalSubsystem::SubscribeSetTooltip()
{
    if (!CachedEventBus.IsValid() || SetTooltipHandle.IsValid()) return;
    SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
    SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SetTooltipConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::InteractSetTooltip;
    SetTooltipConditionAsset->FilterRow.TerminalComparison = EConditionComparison::Equals;
    SetTooltipConditionAsset->CompileCondition();
    if (SetTooltipConditionAsset->GetCondition().IsValid())
    {
        SetTooltipHandle = CachedEventBus->RegisterHandler(
            SetTooltipConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleSetTooltip)
        );
    }
}

void UTerminalSubsystem::SubscribeTestInteractCommand()
{
    if (!CachedEventBus.IsValid() || TerminalCommandHandle.IsValid()) return;
    TerminalCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    TerminalCommandConditionAsset->OperatorType = EConditionOperator::Composite;
    TerminalCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Terminal;
    TerminalCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    TerminalCommandConditionAsset->FilterRow.TerminalType = EOutcomeTerminal::TerminalCommand;
    TerminalCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    TerminalCommandConditionAsset->CompileCondition();
    if (TerminalCommandConditionAsset->GetCondition().IsValid())
    {
        TerminalCommandHandle = CachedEventBus->RegisterHandler(
            TerminalCommandConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UTerminalSubsystem::HandleTestInteractCommand)
        );
    }
}

void UTerminalSubsystem::UnsubscribeTestInteractCommand()
{
    if (!CachedEventBus.IsValid() || !TerminalCommandHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(TerminalCommandHandle);
    TerminalCommandHandle.Invalidate();
    TerminalCommandConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleTestInteractCommand(const FOutcomeEventBase& Outcome)
{
    if (Outcome.OutcomeType == EOutcomeType::Terminal)
    {
        if(Outcome.OutcomeTerminal == EOutcomeTerminal::TerminalCommand)
        {
            const UTerminalCommandPayload* P = Cast<UTerminalCommandPayload>(Outcome.Payload);
            if (!P) return;

            FGuid ObjectItemId = P->ObjectItemId;
            AActor* OwnerActor = P->OwnerActor;
            bool IsEnable = P->IsEnable;


            UInteractiveItemComponent* InteractiveComp = OwnerActor->FindComponentByClass<UInteractiveItemComponent>();

            if (InteractiveComp)
            {
                FGuid InteractiveId = InteractiveComp->GetItemId();

                if (UEventBusSubsystem* EventBus = GetWorld()->GetGameInstance()->GetSubsystem<UEventBusSubsystem>())
                {
                    if (UInteractSetEnabledPayload* P1 = EventBus->CreatePayload<UInteractSetEnabledPayload>())
                    {
                        P1->Setup(InteractiveId, IsEnable);

                        FOutcomeEventBase Outcome;
                        Outcome.Payload = P1;

                        UFloorAssignmentComponent* FloorComp = OwnerActor->FindComponentByClass<UFloorAssignmentComponent>();
                        if (!FloorComp)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("TerminalSubsystem: OwnerActor %s does not have a FloorAssignmentComponent"), *OwnerActor->GetName());
                            return;
                        }
                        EFloorActorType ActorType = FloorComp->ActorType;

                        switch (ActorType)
                        {
                        case EFloorActorType::LightItem:
                        case EFloorActorType::DoorLocks:
                        {
                            Outcome.OutcomeType = EOutcomeType::Interior;
                            Outcome.OutcomeInterior = EOutcomeInterior::InteractSetEnabled;
                            break;
                        }
                        case EFloorActorType::Terminal:
                        {
                            Outcome.OutcomeType = EOutcomeType::Terminal;
                            Outcome.OutcomeTerminal = EOutcomeTerminal::InteractSetEnabled;
                            break;
                        }
                        }

                        EventBus->PublishOutcome(Outcome);
                    }
                }
            }

            //if (const FTerminalInteractRecord* Rec = RegisteredItems.Find(P->ObjectItemId))
            //    ExecuteTestInteractCommandOnOwner(P->ObjectItemId, Rec->OwnerActor.Get(), P->IsEnable);

        }
    }
}

void UTerminalSubsystem::UnsubscribeSetTooltip()
{
    if (!CachedEventBus.IsValid() || !SetTooltipHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(SetTooltipHandle);
    SetTooltipHandle.Invalidate();
    SetTooltipConditionAsset = nullptr;
}

void UTerminalSubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
    const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload);
    if (!P) return;
    if (const FTerminalInteractRecord* Rec = RegisteredItems.Find(P->ItemId))
        ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
}

// ============================================================================
// Subscription management
// ============================================================================

void UTerminalSubsystem::SubscribeAll()
{
    SubscribeRegistration();
    SubscribeInteractCommand();
    SubscribeSetEnabled();
    SubscribeSetRange();
    SubscribeSetTooltip();
    SubscribeTestInteractCommand();
}

void UTerminalSubsystem::UnsubscribeAll()
{
    UnsubscribeRegistration();
    UnsubscribeInteractCommand();
    UnsubscribeSetEnabled();
    UnsubscribeSetRange();
    UnsubscribeSetTooltip();
	UnsubscribeTestInteractCommand();
}

// ============================================================================
// Save / Load (ISaveableSubsystem)
// ============================================================================

void UTerminalSubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    OutData.SubsystemName = GetSaveSubsystemName();

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // Serialize terminals
    TArray<TSharedPtr<FJsonValue>> TerminalArray;
    for (const auto& Pair : Terminals)
    {
        const FGuid& Id = Pair.Key;
        const FTerminalState& State = Pair.Value;

        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        Obj->SetStringField(TEXT("TerminalId"), Id.ToString());
        Obj->SetBoolField(TEXT("bIsLocked"), State.bIsLocked);
        Obj->SetStringField(TEXT("PasswordHash"), State.PasswordHash);
        Obj->SetBoolField(TEXT("bIsAccountLoggedIn"), State.bIsAccountLoggedIn);
        Obj->SetStringField(TEXT("CurrentAccountName"), State.CurrentAccountName);
        Obj->SetBoolField(TEXT("bIsOpen"), State.bIsOpen);
        Obj->SetStringField(TEXT("ProfileName"), State.ProfileName);

        // Capabilities
        TArray<TSharedPtr<FJsonValue>> CapArray;
        for (ETerminalCapability Cap : State.Capabilities)
            CapArray.Add(MakeShared<FJsonValueNumber>(static_cast<uint8>(Cap)));
        Obj->SetArrayField(TEXT("Capabilities"), CapArray);

        // Local files
        TArray<TSharedPtr<FJsonValue>> FilesArray;
        for (const FTerminalFileEntry& File : State.LocalFiles)
        {
            TSharedPtr<FJsonObject> FileObj = MakeShared<FJsonObject>();
            FileObj->SetStringField(TEXT("FileName"), File.FileName);
            FileObj->SetStringField(TEXT("Content"), File.Content);
            FileObj->SetStringField(TEXT("LastModified"), File.LastModified.ToString());
            FileObj->SetStringField(TEXT("ParentPath"), File.ParentPath);
            FilesArray.Add(MakeShared<FJsonValueObject>(FileObj));
        }
        Obj->SetArrayField(TEXT("LocalFiles"), FilesArray);

        // Logs
        TArray<TSharedPtr<FJsonValue>> LogArray;
        for (const FTerminalLogEntry& Log : State.LocalLogs)
        {
            TSharedPtr<FJsonObject> LogJsonObj = MakeShared<FJsonObject>();
            LogJsonObj->SetStringField(TEXT("Timestamp"), Log.Timestamp.ToString());
            LogJsonObj->SetStringField(TEXT("Message"), Log.Message);
            LogArray.Add(MakeShared<FJsonValueObject>(LogJsonObj));
        }
        Obj->SetArrayField(TEXT("LocalLogs"), LogArray);

        TerminalArray.Add(MakeShared<FJsonValueObject>(Obj));
    }
    Root->SetArrayField(TEXT("Terminals"), TerminalArray);

    // Serialize global state
    TSharedPtr<FJsonObject> GlobalObj = MakeShared<FJsonObject>();

    TSharedPtr<FJsonObject> EmailObj = MakeShared<FJsonObject>();
    for (const auto& E : GlobalState.EmailBoxes)
        EmailObj->SetStringField(E.Key, E.Value);
    GlobalObj->SetObjectField(TEXT("EmailBoxes"), EmailObj);

    TSharedPtr<FJsonObject> WebObj = MakeShared<FJsonObject>();
    for (const auto& W : GlobalState.Websites)
        WebObj->SetStringField(W.Key, W.Value);
    GlobalObj->SetObjectField(TEXT("Websites"), WebObj);

    TSharedPtr<FJsonObject> DataObj = MakeShared<FJsonObject>();
    for (const auto& D : GlobalState.SharedData)
        DataObj->SetStringField(D.Key, D.Value);
    GlobalObj->SetObjectField(TEXT("SharedData"), DataObj);

    Root->SetObjectField(TEXT("GlobalState"), GlobalObj);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void UTerminalSubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    bIsLoadComplete = false;

    if (InData.SerializedData.IsEmpty())
    {
        bIsLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        bIsLoadComplete = true;
        return;
    }

    Terminals.Empty();
    GlobalState = FTerminalGlobalState();

    const TArray<TSharedPtr<FJsonValue>>* TerminalArray = nullptr;
    if (Root->TryGetArrayField(TEXT("Terminals"), TerminalArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *TerminalArray)
        {
            const TSharedPtr<FJsonObject>* ObjPtr = nullptr;
            if (!Val->TryGetObject(ObjPtr)) continue;
            const TSharedPtr<FJsonObject>& Obj = *ObjPtr;

            FTerminalState State;
            FGuid::Parse(Obj->GetStringField(TEXT("TerminalId")), State.TerminalId);
            State.bIsLocked = Obj->GetBoolField(TEXT("bIsLocked"));
            State.PasswordHash = Obj->GetStringField(TEXT("PasswordHash"));
            State.bIsAccountLoggedIn = Obj->GetBoolField(TEXT("bIsAccountLoggedIn"));
            State.CurrentAccountName = Obj->GetStringField(TEXT("CurrentAccountName"));
            State.bIsOpen = Obj->GetBoolField(TEXT("bIsOpen"));
            Obj->TryGetStringField(TEXT("ProfileName"), State.ProfileName);

            const TArray<TSharedPtr<FJsonValue>>* CapArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("Capabilities"), CapArray))
            {
                for (const TSharedPtr<FJsonValue>& CapVal : *CapArray)
                {
                    uint8 CapInt = static_cast<uint8>(CapVal->AsNumber());
                    State.Capabilities.Add(static_cast<ETerminalCapability>(CapInt));
                }
            }

            const TArray<TSharedPtr<FJsonValue>>* FilesArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("LocalFiles"), FilesArray))
            {
                for (const TSharedPtr<FJsonValue>& FileVal : *FilesArray)
                {
                    const TSharedPtr<FJsonObject>* FileObjPtr = nullptr;
                    if (!FileVal->TryGetObject(FileObjPtr)) continue;
                    const TSharedPtr<FJsonObject>& FileObj = *FileObjPtr;
                    FTerminalFileEntry Entry;
                    Entry.FileName = FileObj->GetStringField(TEXT("FileName"));
                    Entry.Content = FileObj->GetStringField(TEXT("Content"));
                    FDateTime::Parse(FileObj->GetStringField(TEXT("LastModified")), Entry.LastModified);
                    Entry.ParentPath = FileObj->GetStringField(TEXT("ParentPath"));
                    State.LocalFiles.Add(Entry);
                }
            }

            const TArray<TSharedPtr<FJsonValue>>* LogArray = nullptr;
            if (Obj->TryGetArrayField(TEXT("LocalLogs"), LogArray))
            {
                for (const TSharedPtr<FJsonValue>& LogVal : *LogArray)
                {
                    const TSharedPtr<FJsonObject>* LogJsonObjPtr = nullptr;
                    if (!LogVal->TryGetObject(LogJsonObjPtr)) continue;
                    const TSharedPtr<FJsonObject>& LogJsonObj = *LogJsonObjPtr;
                    FTerminalLogEntry Entry;
                    FDateTime::Parse(LogJsonObj->GetStringField(TEXT("Timestamp")), Entry.Timestamp);
                    Entry.Message = LogJsonObj->GetStringField(TEXT("Message"));
                    State.LocalLogs.Add(Entry);
                }
            }

            Terminals.Add(State.TerminalId, State);
        }
    }

    const TSharedPtr<FJsonObject>* GlobalObjPtr = nullptr;
    if (Root->TryGetObjectField(TEXT("GlobalState"), GlobalObjPtr))
    {
        const TSharedPtr<FJsonObject>& GlobalObj = *GlobalObjPtr;

        const TSharedPtr<FJsonObject>* EmailObjPtr = nullptr;
        if (GlobalObj->TryGetObjectField(TEXT("EmailBoxes"), EmailObjPtr))
        {
            for (const auto& Pair : (*EmailObjPtr)->Values)
                GlobalState.EmailBoxes.Add(Pair.Key, Pair.Value->AsString());
        }

        const TSharedPtr<FJsonObject>* WebObjPtr = nullptr;
        if (GlobalObj->TryGetObjectField(TEXT("Websites"), WebObjPtr))
        {
            for (const auto& Pair : (*WebObjPtr)->Values)
                GlobalState.Websites.Add(Pair.Key, Pair.Value->AsString());
        }

        const TSharedPtr<FJsonObject>* DataObjPtr = nullptr;
        if (GlobalObj->TryGetObjectField(TEXT("SharedData"), DataObjPtr))
        {
            for (const auto& Pair : (*DataObjPtr)->Values)
                GlobalState.SharedData.Add(Pair.Key, Pair.Value->AsString());
        }
    }

    bIsLoadComplete = true;
    BroadcastGlobalState();
}

// ============================================================================
// Internal helpers
// ============================================================================

bool UTerminalSubsystem::IsTerminalCapable(const FGuid& TerminalId, ETerminalCapability Capability) const
{
    const FTerminalState* State = Terminals.Find(TerminalId);
    if (!State) return false;
    return State->Capabilities.Contains(Capability);
}

void UTerminalSubsystem::UpdateTerminalState(const FGuid& TerminalId, const FTerminalState& NewState)
{
    if (FTerminalState* State = Terminals.Find(TerminalId))
    {
        *State = NewState;
        BroadcastTerminalState(TerminalId);
    }
}

void UTerminalSubsystem::BroadcastTerminalState(const FGuid& TerminalId)
{
    if (Terminals.Contains(TerminalId))
        OnTerminalStateChanged.Broadcast(TerminalId, Terminals[TerminalId]);
}

void UTerminalSubsystem::BroadcastGlobalState()
{
    OnTerminalGlobalStateChanged.Broadcast(GlobalState);
}

