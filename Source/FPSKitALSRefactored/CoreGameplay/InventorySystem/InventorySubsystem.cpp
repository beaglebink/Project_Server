#include "InventorySubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"         
#include "Inventory/AC_Inventory.h"
#include "Inventory/AC_Container.h"
#include "GameSaveSubsystem.h"
#include "JsonObjectConverter.h"
#include "InventoryPayloads.h"
#include "PlayerController_I.h"

// ─────────────────────────────────────────────────────────────────────────────
// Static JSON helpers
// ─────────────────────────────────────────────────────────────────────────────

static TSharedPtr<FJsonObject> SnapshotToJson(const FInventorySnapshot& Snapshot)
{
    TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

    TArray<TSharedPtr<FJsonValue>> ItemsArray;
    ItemsArray.Reserve(Snapshot.Items.Num());
    for (const FS_Item& Item : Snapshot.Items)
    {
        TSharedPtr<FJsonObject> ItemJson = FJsonObjectConverter::UStructToJsonObject(Item);
        if (ItemJson.IsValid())
        {
            ItemsArray.Add(MakeShared<FJsonValueObject>(ItemJson));
        }
    }
    Obj->SetArrayField(TEXT("Items"), ItemsArray);
    Obj->SetNumberField(TEXT("TotalWeight"), Snapshot.TotalWeight);
    Obj->SetNumberField(TEXT("TotalMoney"), Snapshot.TotalMoney);
    return Obj;
}

static bool SnapshotFromJson(const TSharedPtr<FJsonObject>& Obj, FInventorySnapshot& OutSnapshot)
{
    if (!Obj.IsValid()) return false;

    OutSnapshot.Reset();

    const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
    if (Obj->TryGetArrayField(TEXT("Items"), ItemsArray))
    {
        for (const TSharedPtr<FJsonValue>& Val : *ItemsArray)
        {
            if (!Val.IsValid() || Val->Type != EJson::Object) continue;

            FS_Item Item;
            if (FJsonObjectConverter::JsonObjectToUStruct(
                Val->AsObject().ToSharedRef(),
                FS_Item::StaticStruct(),
                &Item, 0, 0))
            {
                OutSnapshot.Items.Add(Item);
            }
        }
    }

    double W = 0.0, M = 0.0;
    if (Obj->TryGetNumberField(TEXT("TotalWeight"), W)) OutSnapshot.TotalWeight = static_cast<float>(W);
    if (Obj->TryGetNumberField(TEXT("TotalMoney"), M)) OutSnapshot.TotalMoney = static_cast<float>(M);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UGameSaveSubsystem>();
    Collection.InitializeDependency(UEventBusSubsystem::StaticClass());
    Super::Initialize(Collection);

    if (UGameSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UGameSaveSubsystem>())
    {
        SaveSys->RegisterSaveableSubsystem(this);
    }

    CachedEventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();
    if (!CachedEventBus.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("InventorySubsystem: CachedEventBus is NULL after Initialize - all subscriptions skipped!"));
        return;
    }

    SubscribeRegistration();
    SubscribeInteractCommand();
    SubscribeSetRange();
    SubscribeSetTooltip();
    SubscribeInventorySnapshot();
    SubscribeContainerSnapshot();

    if (ItemAcquiredCondition) SubscribeItemAcquired();

    PostLoadMapClearLoadingHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this, &UInventorySubsystem::OnPostLoadMap_ClearLoadingFlag);
}

void UInventorySubsystem::Deinitialize()
{
    if (PostLoadMapClearLoadingHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapClearLoadingHandle);
        PostLoadMapClearLoadingHandle.Reset();
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UGameSaveSubsystem* SaveSys = GI->GetSubsystem<UGameSaveSubsystem>())
        {
            SaveSys->UnregisterSaveableSubsystem(this);
        }
    }

    UnsubscribeAll();
    CachedEventBus.Reset();
    Super::Deinitialize();
}

void UInventorySubsystem::UnsubscribeAll()
{
    UnsubscribeItemAcquired();
    UnsubscribeInteractCommand();
    UnsubscribeSetRange();
    UnsubscribeSetTooltip();
    UnsubscribeRegistration();
    UnsubscribeInventorySnapshot();
    UnsubscribeContainerSnapshot();
}

// ─────────────────────────────────────────────────────────────────────────────
// ItemAcquired
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::SetItemAcquiredCondition(UOutcomeConditionAsset* NewCondition)
{
    if (ItemAcquiredCondition == NewCondition) return;
    ItemAcquiredCondition = NewCondition;
    if (ItemAcquiredHandle.IsValid()) UnsubscribeItemAcquired();
    if (ItemAcquiredCondition) SubscribeItemAcquired();
}

void UInventorySubsystem::SubscribeItemAcquired()
{
    if (ItemAcquiredHandle.IsValid() || !ItemAcquiredCondition) return;
    if (UEventBusSubsystem* EventBus = CachedEventBus.Get())
    {
        ItemAcquiredHandle = EventBus->RegisterHandler(
            ItemAcquiredCondition,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleItemAcquired));
    }
}

void UInventorySubsystem::UnsubscribeItemAcquired()
{
    if (!ItemAcquiredHandle.IsValid()) return;
    if (UEventBusSubsystem* EventBus = CachedEventBus.Get()) EventBus->UnregisterHandler(ItemAcquiredHandle);
    ItemAcquiredHandle.Invalidate();
}

void UInventorySubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
    if (ItemAcquiredCondition)
    {
        auto Query = ItemAcquiredCondition->GetCondition();
        if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
    }
    OnItemAcquired.Broadcast(Outcome);
}

// ─────────────────────────────────────────────────────────────────────────────
// Registration
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::SubscribeRegistration()
{
    if (!CachedEventBus.IsValid()) return;
    if (RegisteredRegisterHandle.IsValid() || UnregisteredRegisterHandle.IsValid()) return;

    RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
    RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    RegisteredConditionAsset->FilterRow.ObjectType = EOutcomeInventory::InteractRegistered;
    RegisteredConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    RegisteredConditionAsset->CompileCondition();

    UnregisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    UnregisteredConditionAsset->OperatorType = EConditionOperator::Composite;
    UnregisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    UnregisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    UnregisteredConditionAsset->FilterRow.ObjectType = EOutcomeInventory::InteractUnregistered;
    UnregisteredConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    UnregisteredConditionAsset->CompileCondition();

    if (RegisteredConditionAsset->GetCondition().IsValid())
    {
        RegisteredRegisterHandle = CachedEventBus->RegisterHandler(
            RegisteredConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractRegistration));
    }
    if (UnregisteredConditionAsset->GetCondition().IsValid())
    {
        UnregisteredRegisterHandle = CachedEventBus->RegisterHandler(
            UnregisteredConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractRegistration));
    }
}

void UInventorySubsystem::UnsubscribeRegistration()
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

void UInventorySubsystem::AddRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
    FInteractiveSubsystemMethods::AddRegistrationListener(ItemId, Listener);
}

void UInventorySubsystem::RemoveRegistrationListener(const FGuid& ItemId, UInteractiveItemComponent* Listener)
{
    FInteractiveSubsystemMethods::RemoveRegistrationListener(ItemId, Listener);
}

void UInventorySubsystem::HandleInteractRegistration(const FOutcomeEventBase& Outcome)
{
    if (UInteractItemRegistrationPayload* P = Cast<UInteractItemRegistrationPayload>(Outcome.Payload))
    {
        const FGuid Id = P->ItemId;
        AActor* Owner = P->GetOwnerActor();

        if (Outcome.OutcomeInventory == EOutcomeInventory::InteractRegistered)
        {
            FInventoryInteractItemRecord R;
            R.ItemId = Id;
            R.OwnerActor = Owner;
            R.InteractionRange = P->InteractionRange;
            RegisteredItems.Add(Id, R);

            if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(Id))
            {
                for (auto It = List->CreateIterator(); It; ++It)
                {
                    if (UInteractiveItemComponent* Comp = It->Get()) Comp->OnRegisteredBySubsystem(P);
                }
                RegistrationListeners.Remove(Id);
            }
        }
        else if (Outcome.OutcomeInventory == EOutcomeInventory::InteractUnregistered)
        {
            RegisteredItems.Remove(Id);
            if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(Id))
            {
                for (auto It = List->CreateIterator(); It; ++It)
                {
                    if (UInteractiveItemComponent* Comp = It->Get()) Comp->OnUnregisteredBySubsystem(P);
                }
                RegistrationListeners.Remove(Id);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// InteractCommand
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::SubscribeInteractCommand()
{
    if (!CachedEventBus.IsValid() || InteractCommandHandle.IsValid()) return;

    InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
    InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    InteractCommandConditionAsset->CompileCondition();

    if (InteractCommandConditionAsset->GetCondition().IsValid())
    {
        InteractCommandHandle = CachedEventBus->RegisterHandler(
            InteractCommandConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractCommand));
    }
}

void UInventorySubsystem::UnsubscribeInteractCommand()
{
    if (!CachedEventBus.IsValid() || !InteractCommandHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(InteractCommandHandle);
    InteractCommandHandle.Invalidate();
    InteractCommandConditionAsset = nullptr;
}

void UInventorySubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
    if (const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload))
    {
        if (const FInventoryInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
        {
            if (AActor* Owner = Rec->OwnerActor.Get())
            {
                ExecuteInteractCommandOnOwner(P->ItemId, Owner, P->Picker);
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SetRange / SetTooltip
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::SubscribeSetRange()
{
    if (!CachedEventBus.IsValid() || SetRangeHandle.IsValid()) return;

    SetRangeConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SetRangeConditionAsset->OperatorType = EConditionOperator::Composite;
    SetRangeConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    SetRangeConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SetRangeConditionAsset->FilterRow.ObjectType = EOutcomeInventory::InteractSetRange;
    SetRangeConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    SetRangeConditionAsset->CompileCondition();

    if (SetRangeConditionAsset->GetCondition().IsValid())
    {
        SetRangeHandle = CachedEventBus->RegisterHandler(
            SetRangeConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSetRange));
    }
}

void UInventorySubsystem::UnsubscribeSetRange()
{
    if (!CachedEventBus.IsValid() || !SetRangeHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(SetRangeHandle);
    SetRangeHandle.Invalidate();
    SetRangeConditionAsset = nullptr;
}

void UInventorySubsystem::HandleSetRange(const FOutcomeEventBase& Outcome)
{
    if (const UInteractSetRangePayload* P = Cast<UInteractSetRangePayload>(Outcome.Payload))
    {
        if (const FInventoryInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
        {
            ExecuteSetRangeOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewRange);
        }
    }
}

void UInventorySubsystem::SubscribeSetTooltip()
{
    if (!CachedEventBus.IsValid() || SetTooltipHandle.IsValid()) return;

    SetTooltipConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SetTooltipConditionAsset->OperatorType = EConditionOperator::Composite;
    SetTooltipConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    SetTooltipConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SetTooltipConditionAsset->FilterRow.ObjectType = EOutcomeInventory::InteractSetTooltip;
    SetTooltipConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    SetTooltipConditionAsset->CompileCondition();

    if (SetTooltipConditionAsset->GetCondition().IsValid())
    {
        SetTooltipHandle = CachedEventBus->RegisterHandler(
            SetTooltipConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSetTooltip));
    }
}

void UInventorySubsystem::UnsubscribeSetTooltip()
{
    if (!CachedEventBus.IsValid() || !SetTooltipHandle.IsValid()) return;
    CachedEventBus->UnregisterHandler(SetTooltipHandle);
    SetTooltipHandle.Invalidate();
    SetTooltipConditionAsset = nullptr;
}

void UInventorySubsystem::HandleSetTooltip(const FOutcomeEventBase& Outcome)
{
    if (const UInteractSetTooltipPayload* P = Cast<UInteractSetTooltipPayload>(Outcome.Payload))
    {
        if (const FInventoryInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
        {
            ExecuteSetTooltipOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->NewTooltip);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Player inventory snapshot (Save/RestoreInventoryRequest)
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::SubscribeInventorySnapshot()
{
    if (!CachedEventBus.IsValid()) return;
    if (SaveInventoryHandle.IsValid() || RestoreInventoryHandle.IsValid()) return;

    SaveInventoryConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SaveInventoryConditionAsset->OperatorType = EConditionOperator::Composite;
    SaveInventoryConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    SaveInventoryConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SaveInventoryConditionAsset->FilterRow.ObjectType = EOutcomeInventory::SaveInventoryRequest;
    SaveInventoryConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    SaveInventoryConditionAsset->CompileCondition();

    if (SaveInventoryConditionAsset->GetCondition().IsValid())
    {
        SaveInventoryHandle = CachedEventBus->RegisterHandler(
            SaveInventoryConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSaveInventoryRequest));
    }

    RestoreInventoryConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    RestoreInventoryConditionAsset->OperatorType = EConditionOperator::Composite;
    RestoreInventoryConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    RestoreInventoryConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    RestoreInventoryConditionAsset->FilterRow.ObjectType = EOutcomeInventory::RestoreInventoryRequest;
    RestoreInventoryConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    RestoreInventoryConditionAsset->CompileCondition();

    if (RestoreInventoryConditionAsset->GetCondition().IsValid())
    {
        RestoreInventoryHandle = CachedEventBus->RegisterHandler(
            RestoreInventoryConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleRestoreInventoryRequest));
    }
}

void UInventorySubsystem::UnsubscribeInventorySnapshot()
{
    if (!CachedEventBus.IsValid()) return;

    if (SaveInventoryHandle.IsValid())
    {
        CachedEventBus->UnregisterHandler(SaveInventoryHandle);
        SaveInventoryHandle.Invalidate();
    }
    if (RestoreInventoryHandle.IsValid())
    {
        CachedEventBus->UnregisterHandler(RestoreInventoryHandle);
        RestoreInventoryHandle.Invalidate();
    }

    SaveInventoryConditionAsset = nullptr;
    RestoreInventoryConditionAsset = nullptr;
}

void UInventorySubsystem::HandleSaveInventoryRequest(const FOutcomeEventBase& Outcome)
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    UAC_Inventory* Inventory = Pawn->FindComponentByClass<UAC_Inventory>();
    if (!Inventory || !Inventory->ContainerComponent) return;

    SavedInventorySnapshot.Items = Inventory->ContainerComponent->Items;
    SavedInventorySnapshot.TotalWeight = Inventory->ContainerComponent->GetWeight();
    SavedInventorySnapshot.TotalMoney = Inventory->ContainerComponent->GetMoney();
    bHasSavedInventorySnapshot = true;
}

void UInventorySubsystem::HandleRestoreInventoryRequest(const FOutcomeEventBase& Outcome)
{
    if (!bHasSavedInventorySnapshot) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    UAC_Inventory* Inventory = Pawn->FindComponentByClass<UAC_Inventory>();
    if (!Inventory || !Inventory->ContainerComponent) return;

    Inventory->ContainerComponent->Items = SavedInventorySnapshot.Items;
    Inventory->ContainerComponent->SetWeight(SavedInventorySnapshot.TotalWeight);
    Inventory->ContainerComponent->SetMoney(SavedInventorySnapshot.TotalMoney);

    SavedInventorySnapshot.Reset();
    bHasSavedInventorySnapshot = false;

    UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Restored player inventory snapshot"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Containers (world)
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::SubscribeContainerSnapshot()
{
    if (!CachedEventBus.IsValid() || SaveContainerHandle.IsValid()) return;

    SaveContainerConditionAsset = NewObject<UOutcomeConditionAsset>(this);
    SaveContainerConditionAsset->OperatorType = EConditionOperator::Composite;
    SaveContainerConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
    SaveContainerConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
    SaveContainerConditionAsset->FilterRow.ObjectType = EOutcomeInventory::SaveContainerRequest;
    SaveContainerConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
    SaveContainerConditionAsset->CompileCondition();

    if (SaveContainerConditionAsset->GetCondition().IsValid())
    {
        SaveContainerHandle = CachedEventBus->RegisterHandler(
            SaveContainerConditionAsset,
            FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSaveContainerRequest));
    }
}

void UInventorySubsystem::UnsubscribeContainerSnapshot()
{
    if (CachedEventBus.IsValid() && SaveContainerHandle.IsValid())
    {
        CachedEventBus->UnregisterHandler(SaveContainerHandle);
        SaveContainerHandle.Invalidate();
    }
    SaveContainerConditionAsset = nullptr;
}

void UInventorySubsystem::SetWeaponData(const FString& AmmoData, const FString& WeaponData)
{
    SerializedAmmoData = AmmoData;
    SerializedWeaponsData = WeaponData;
}

void UInventorySubsystem::GetWeaponData(FString& AmmoData, FString& WeaponData)
{
    AmmoData = SerializedAmmoData;
    WeaponData = SerializedWeaponsData;
}

void UInventorySubsystem::HandleSaveContainerRequest(const FOutcomeEventBase& Outcome)
{
    const UInventoryContainerPayload* P = Cast<UInventoryContainerPayload>(Outcome.Payload);
    if (!P || !P->ItemId.IsValid()) return;

    // Пока идёт LoadGame, EndPlay старых контейнеров предыдущей сцены
    // шлёт SaveContainerRequest — игнорируем, чтобы не перезаписать загруженные данные.
    if (bIsLoadingFromDisk)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("InventorySubsystem: Ignoring SaveContainerRequest during load (%s)"),
            *P->ItemId.ToString());
        return;
    }

    ContainerSnapshots.Add(P->ItemId, P->Snapshot);
    UE_LOG(LogTemp, Log,
        TEXT("InventorySubsystem: Saved container %s (%d items)"),
        *P->ItemId.ToString(), P->Snapshot.Items.Num());
}

void UInventorySubsystem::RegisterLiveContainer(UAC_Container* Container)
{
    if (!Container) return;
    const FGuid Id = Container->GetActorItemId();
    if (!Id.IsValid()) return;
    LiveContainers.Add(Id, Container);
}

void UInventorySubsystem::UnregisterLiveContainer(UAC_Container* Container)
{
    if (!Container) return;
    const FGuid Id = Container->GetActorItemId();
    if (!Id.IsValid()) return;
    LiveContainers.Remove(Id);
}

bool UInventorySubsystem::HasContainerSnapshot(const FGuid& ItemId) const
{
    return ContainerSnapshots.Contains(ItemId);
}

bool UInventorySubsystem::GetContainerSnapshot(const FGuid& ItemId, FInventorySnapshot& OutSnapshot) const
{
    if (const FInventorySnapshot* Found = ContainerSnapshots.Find(ItemId))
    {
        OutSnapshot = *Found;
        return true;
    }
    return false;
}

void UInventorySubsystem::RefreshContainerSnapshotsFromLive()
{
    for (auto It = LiveContainers.CreateIterator(); It; ++It)
    {
        UAC_Container* Container = It->Value.Get();
        if (!Container || !IsValid(Container) || Container->IsPlayerContainer())
        {
            It.RemoveCurrent();
            continue;
        }

        FInventorySnapshot Snap;
        Snap.Items = Container->Items;
        Snap.TotalWeight = Container->GetWeight();
        Snap.TotalMoney = Container->GetMoney();
        ContainerSnapshots.Add(It->Key, Snap);
    }
}

void UInventorySubsystem::RefreshPlayerInventorySnapshot()
{
    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    UAC_Inventory* Inventory = Pawn->FindComponentByClass<UAC_Inventory>();
    if (!Inventory || !Inventory->ContainerComponent) return;

    SavedInventorySnapshot.Items = Inventory->ContainerComponent->Items;
    SavedInventorySnapshot.TotalWeight = Inventory->ContainerComponent->GetWeight();
    SavedInventorySnapshot.TotalMoney = Inventory->ContainerComponent->GetMoney();
    bHasSavedInventorySnapshot = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// ISaveableSubsystem
// ─────────────────────────────────────────────────────────────────────────────

void UInventorySubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
    UWorld* W = GetWorld();
    if (!W)
    {
        return;
    }

    OutData.SubsystemName = GetSaveSubsystemName();

    APlayerController* PC = W->GetFirstPlayerController();
    if (PC && PC->GetClass()->ImplementsInterface(UPlayerController_I::StaticClass()))
    {
        //IPlayerController_I::Execute_SetLoadScreen(PC, true);
        IPlayerController_I::Execute_StoreWeaponState(PC);
    }

    RefreshPlayerInventorySnapshot();
    RefreshContainerSnapshotsFromLive();

    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    // Player (root-level, обратная совместимость)
    {
        TSharedPtr<FJsonObject> PlayerJson = SnapshotToJson(SavedInventorySnapshot);
        Root->SetArrayField(TEXT("Items"), PlayerJson->GetArrayField(TEXT("Items")));
        Root->SetNumberField(TEXT("TotalWeight"), PlayerJson->GetNumberField(TEXT("TotalWeight")));
        Root->SetNumberField(TEXT("TotalMoney"), PlayerJson->GetNumberField(TEXT("TotalMoney")));
        Root->SetBoolField(TEXT("HasPlayerInventory"), bHasSavedInventorySnapshot);
    }

    // Containers
    {
        TSharedPtr<FJsonObject> ContainersObj = MakeShared<FJsonObject>();
        for (const auto& Pair : ContainerSnapshots)
        {
            ContainersObj->SetObjectField(Pair.Key.ToString(), SnapshotToJson(Pair.Value));
        }
        Root->SetObjectField(TEXT("Containers"), ContainersObj);
    }

    // ── Weapon / Ammo serialized state ──
    Root->SetStringField(TEXT("SerializedAmmoData"), SerializedAmmoData);
    Root->SetStringField(TEXT("SerializedWeaponsData"), SerializedWeaponsData);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    OutData.SerializedData = Output;
}

void UInventorySubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
    IsLoadComplete = false;
    bIsLoadingFromDisk = true;

    SavedInventorySnapshot.Reset();
    bHasSavedInventorySnapshot = false;
    ContainerSnapshots.Empty();

    // Сбрасываем оружейные данные — если в сейве их нет, не тащим состояние предыдущей сессии.
    SerializedAmmoData.Empty();
    SerializedWeaponsData.Empty();

    if (InData.SerializedData.IsEmpty())
    {
        IsLoadComplete = true;
        return;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        IsLoadComplete = true;
        return;
    }

    // Player
    {
        FInventorySnapshot PlayerSnapshot;
        SnapshotFromJson(Root, PlayerSnapshot);

        bool bHasPlayer = true;
        Root->TryGetBoolField(TEXT("HasPlayerInventory"), bHasPlayer);

        if (bHasPlayer)
        {
            SavedInventorySnapshot = PlayerSnapshot;
            bHasSavedInventorySnapshot = true;
        }
    }

    // Containers
    {
        const TSharedPtr<FJsonObject>* ContainersPtr = nullptr;
        if (Root->TryGetObjectField(TEXT("Containers"), ContainersPtr) && ContainersPtr && (*ContainersPtr).IsValid())
        {
            for (const auto& Pair : (*ContainersPtr)->Values)
            {
                FGuid ItemId;
                if (!FGuid::Parse(Pair.Key, ItemId)) continue;

                if (Pair.Value.IsValid() && Pair.Value->Type == EJson::Object)
                {
                    FInventorySnapshot Snap;
                    SnapshotFromJson(Pair.Value->AsObject(), Snap);
                    ContainerSnapshots.Add(ItemId, Snap);
                }
            }
        }
    }

    // ── Weapon / Ammo serialized state ──
    Root->TryGetStringField(TEXT("SerializedAmmoData"), SerializedAmmoData);
    Root->TryGetStringField(TEXT("SerializedWeaponsData"), SerializedWeaponsData);

    IsLoadComplete = true;
}

void UInventorySubsystem::OnPostLoadMap_ClearLoadingFlag(UWorld* LoadedWorld)
{
    if (!LoadedWorld) return;
    if (LoadedWorld->WorldType != EWorldType::Game && LoadedWorld->WorldType != EWorldType::PIE) return;

    if (bIsLoadingFromDisk)
    {
        bIsLoadingFromDisk = false;
        UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Load-from-disk flag cleared after PostLoadMap"));
    }
}