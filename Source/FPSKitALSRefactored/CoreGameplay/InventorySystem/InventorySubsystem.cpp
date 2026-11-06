#include "InventorySubsystem.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "ItemAcquiredPayload.h"
#include "../InteractionSystem/InteractItemRegistrationPayload.h"
#include "../InteractionSystem/InteractiveItemComponent.h"
#include "../InteractionSystem/InteractCommandPayload.h"
#include "../InteractionSystem/InteractSetEnabledPayload.h"
#include "../InteractionSystem/InteractSetRangePayload.h"
#include "../InteractionSystem/InteractSetTooltipPayload.h"
#include "Inventory/AC_Inventory.h"
#include "Inventory/AC_Container.h"
#include "JsonObjectConverter.h"
#include <GameSaveSubsystem.h>

void UInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// GameSaveSubsystem должен существовать до нас
	Collection.InitializeDependency<UGameSaveSubsystem>();

	Collection.InitializeDependency(UEventBusSubsystem::StaticClass());
	Super::Initialize(Collection);

	// Регистрируемся в save-системе
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

	// Подписка на save/restore из прошлой задачи
	SubscribeInventorySnapshot();

	if (ItemAcquiredCondition) SubscribeItemAcquired();
}

void UInventorySubsystem::Deinitialize()
{
	// Отмена регистрации в save-системе
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UGameSaveSubsystem* SaveSys = GI->GetSubsystem<UGameSaveSubsystem>())
		{
			SaveSys->UnregisterSaveableSubsystem(this);
		}
	}

	UnsubscribeAll();       // уже включает UnsubscribeInventorySnapshot
	CachedEventBus.Reset();
	Super::Deinitialize();
}

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
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleItemAcquired)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to ItemAcquired (handle=%u)"), ItemAcquiredHandle.GetId());
	}
}

void UInventorySubsystem::UnsubscribeItemAcquired()
{
	if (!ItemAcquiredHandle.IsValid()) return;
	if (UEventBusSubsystem* EventBus = CachedEventBus.Get()) EventBus->UnregisterHandler(ItemAcquiredHandle);
	ItemAcquiredHandle.Invalidate();
}

void UInventorySubsystem::SubscribeRegistration()
{
	if (!CachedEventBus.IsValid()) return;
	if (RegisteredRegisterHandle.IsValid() || UnregisteredRegisterHandle.IsValid()) return;

	// InteractRegistered — Object category
	RegisteredConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	RegisteredConditionAsset->OperatorType = EConditionOperator::Composite;
	RegisteredConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
	RegisteredConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->FilterRow.ObjectType = EOutcomeInventory::InteractRegistered;
	RegisteredConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
	RegisteredConditionAsset->CompileCondition();

	// InteractUnregistered — Object category
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
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to InteractRegistered (handle=%u)"), RegisteredRegisterHandle.GetId());
	}

	if (UnregisteredConditionAsset->GetCondition().IsValid())
	{
		UnregisteredRegisterHandle = CachedEventBus->RegisterHandler(
			UnregisteredConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractRegistration)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to InteractUnregistered (handle=%u)"), UnregisteredRegisterHandle.GetId());
	}

	// Рекомендуется вызвать SubscribeInteractCommand() при инициализации
	//SubscribeInteractCommand();
}

void UInventorySubsystem::SubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid()) return;

	InteractCommandConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	InteractCommandConditionAsset->OperatorType = EConditionOperator::Composite;
	// Подписка на Object/Inventory-тип команд
	InteractCommandConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
	InteractCommandConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	InteractCommandConditionAsset->CompileCondition();

	if (InteractCommandConditionAsset->GetCondition().IsValid())
	{
		InteractCommandHandle = CachedEventBus->RegisterHandler(
			InteractCommandConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleInteractCommand)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to InteractCommand (handle=%u)"), InteractCommandHandle.GetId());
	}
}
/*
void UInventorySubsystem::SubscribeSetEnabled()
{
	// Диагностическое логирование — чтобы точно видеть, почему обработчик может не зарегистрироваться.
	if (!CachedEventBus.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("InventorySubsystem::SubscribeSetEnabled - CachedEventBus is NULL, will skip subscription"));
		return;
	}
	if (SetEnabledHandle.IsValid())
	{
		UE_LOG(LogTemp, Verbose, TEXT("InventorySubsystem::SubscribeSetEnabled - already subscribed (handle=%u)"), SetEnabledHandle.GetId());
		return;
	}

	SetEnabledConditionAsset = NewObject<UOutcomeConditionAsset>(this);
	SetEnabledConditionAsset->OperatorType = EConditionOperator::Composite;
	SetEnabledConditionAsset->FilterRow.OutcomeType = EOutcomeType::Inventory;
	SetEnabledConditionAsset->FilterRow.OutcomeTypeComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->FilterRow.ObjectType = EOutcomeInventory::InteractSetEnabled;
	SetEnabledConditionAsset->FilterRow.ObjectComparison = EConditionComparison::Equals;
	SetEnabledConditionAsset->CompileCondition();

	// Логируем описание условия для отладки
	UE_LOG(LogTemp, Verbose, TEXT("InventorySubsystem: SetEnabled condition description: %s"), *SetEnabledConditionAsset->GetConditionDescription());

	if (SetEnabledConditionAsset->GetCondition().IsValid())
	{
		SetEnabledHandle = CachedEventBus->RegisterHandler(
			SetEnabledConditionAsset,
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSetEnabled)
		);
		if (SetEnabledHandle.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to SetEnabled (handle=%u)"), SetEnabledHandle.GetId());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InventorySubsystem: RegisterHandler returned invalid handle for SetEnabled!"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("InventorySubsystem: SetEnabledCondition compiled to INVALID -> handler NOT registered"));
	}
}
*/
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
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSetRange)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to SetRange (handle=%u)"), SetRangeHandle.GetId());
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
			FOutcomeHandlerDelegate::CreateUObject(this, &UInventorySubsystem::HandleSetTooltip)
		);
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to SetTooltip (handle=%u)"), SetTooltipHandle.GetId());
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

	RegisteredConditionAsset   = nullptr;
	UnregisteredConditionAsset = nullptr;
}

void UInventorySubsystem::UnsubscribeInteractCommand()
{
	if (!CachedEventBus.IsValid()) return;
	if (InteractCommandHandle.IsValid())
	{
		CachedEventBus->UnregisterHandler(InteractCommandHandle);
		InteractCommandHandle.Invalidate();
	}
	InteractCommandConditionAsset = nullptr;
}
/*
void UInventorySubsystem::UnsubscribeSetEnabled()
{
	if (!CachedEventBus.IsValid() || !SetEnabledHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetEnabledHandle);
	SetEnabledHandle.Invalidate();
	SetEnabledConditionAsset = nullptr;
}
*/
void UInventorySubsystem::UnsubscribeSetRange()
{
	if (!CachedEventBus.IsValid() || !SetRangeHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetRangeHandle);
	SetRangeHandle.Invalidate();
	SetRangeConditionAsset = nullptr;
}

void UInventorySubsystem::UnsubscribeSetTooltip()
{
	if (!CachedEventBus.IsValid() || !SetTooltipHandle.IsValid()) return;
	CachedEventBus->UnregisterHandler(SetTooltipHandle);
	SetTooltipHandle.Invalidate();
	SetTooltipConditionAsset = nullptr;
}

void UInventorySubsystem::UnsubscribeAll()
{
	UnsubscribeItemAcquired();
	UnsubscribeInteractCommand();
	//UnsubscribeSetEnabled();
	UnsubscribeSetRange();
	UnsubscribeSetTooltip();
	UnsubscribeRegistration();
	UnsubscribeInventorySnapshot();
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
		AActor* Owner  = P->GetOwnerActor();
		FString OwnerName = Owner ? Owner->GetName() : FString(TEXT("Unknown"));

		if (Outcome.OutcomeInventory == EOutcomeInventory::InteractRegistered)
		{
			FInventoryInteractItemRecord R;
			R.ItemId = Id;
			R.OwnerActor = Owner;
			R.InteractionRange = P->InteractionRange;
			RegisteredItems.Add(Id, R);

			UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Registered interactive item owned by '%s' (range=%.1f)"),
				*OwnerName, P->InteractionRange);

			if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(Id))
			{
				for (auto It = List->CreateIterator(); It; ++It)
				{
					if (UInteractiveItemComponent* Comp = It->Get())
					{
						Comp->OnRegisteredBySubsystem(P);
					}
				}
				RegistrationListeners.Remove(Id);
			}
		}
		else if (Outcome.OutcomeInventory == EOutcomeInventory::InteractUnregistered)
		{
			FString NameToLog = OwnerName;
			if (RegisteredItems.Contains(Id))
			{
				AActor* RegOwner = RegisteredItems[Id].OwnerActor.Get();
				if (RegOwner) NameToLog = RegOwner->GetName();
				RegisteredItems.Remove(Id);
			}
			else if (NameToLog == "Unknown")
			{
				NameToLog = Id.ToString();
			}

			UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Unregistered interactive item owned by '%s'"), *NameToLog);

			if (TArray<TWeakObjectPtr<UInteractiveItemComponent>>* List = RegistrationListeners.Find(Id))
			{
				for (auto It = List->CreateIterator(); It; ++It)
				{
					if (UInteractiveItemComponent* Comp = It->Get())
					{
						Comp->OnUnregisteredBySubsystem(P);
					}
				}
				RegistrationListeners.Remove(Id);
			}
		}
	}
}

void UInventorySubsystem::SubscribeInventorySnapshot()
{
	if (!CachedEventBus.IsValid()) return;
	if (SaveInventoryHandle.IsValid() || RestoreInventoryHandle.IsValid()) return;

	// --- Save request ---
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
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to SaveInventoryRequest (handle=%u)"),
			SaveInventoryHandle.GetId());
	}

	// --- Restore request ---
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
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Subscribed to RestoreInventoryRequest (handle=%u)"),
			RestoreInventoryHandle.GetId());
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
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("InventorySubsystem: Cannot save inventory — player pawn is NULL"));
		return;
	}

	UAC_Inventory* Inventory = Pawn->FindComponentByClass<UAC_Inventory>();
	if (!Inventory || !Inventory->ContainerComponent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("InventorySubsystem: Cannot save inventory — UAC_Inventory or ContainerComponent missing on pawn"));
		return;
	}

	// Захватываем состояние. Меняем «состояние подсистемы» через обработчик события.
	SavedInventorySnapshot.Items = Inventory->ContainerComponent->Items;
	SavedInventorySnapshot.TotalWeight = Inventory->ContainerComponent->GetWeight();
	SavedInventorySnapshot.TotalMoney = Inventory->ContainerComponent->GetMoney();
	bHasSavedInventorySnapshot = true;

	UE_LOG(LogTemp, Log,
		TEXT("InventorySubsystem: Saved inventory snapshot (%d items, weight=%.2f, money=%.2f)"),
		SavedInventorySnapshot.Items.Num(),
		SavedInventorySnapshot.TotalWeight,
		SavedInventorySnapshot.TotalMoney);
}

void UInventorySubsystem::HandleRestoreInventoryRequest(const FOutcomeEventBase& Outcome)
{
	if (!bHasSavedInventorySnapshot)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("InventorySubsystem: RestoreInventoryRequest received, but no saved snapshot exists"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("InventorySubsystem: Cannot restore inventory — player pawn is NULL (snapshot kept for retry)"));
		return;
	}

	UAC_Inventory* Inventory = Pawn->FindComponentByClass<UAC_Inventory>();
	if (!Inventory || !Inventory->ContainerComponent)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("InventorySubsystem: Cannot restore inventory — UAC_Inventory or ContainerComponent missing on pawn (snapshot kept)"));
		return;
	}

	// Применяем снапшот к инвентарю игрока.
	Inventory->ContainerComponent->Items = SavedInventorySnapshot.Items;
	Inventory->ContainerComponent->SetWeight(SavedInventorySnapshot.TotalWeight);
	Inventory->ContainerComponent->SetMoney(SavedInventorySnapshot.TotalMoney);

	UE_LOG(LogTemp, Log,
		TEXT("InventorySubsystem: Restored inventory snapshot (%d items, weight=%.2f, money=%.2f)"),
		SavedInventorySnapshot.Items.Num(),
		SavedInventorySnapshot.TotalWeight,
		SavedInventorySnapshot.TotalMoney);

	// Успешно применили — очищаем, чтобы избежать повторного применения
	// (например, при загрузке сейва после возврата в главное меню).
	SavedInventorySnapshot.Reset();
	bHasSavedInventorySnapshot = false;
}

void UInventorySubsystem::HandleItemAcquired(const FOutcomeEventBase& Outcome)
{
	if (ItemAcquiredCondition)
	{
		auto Query = ItemAcquiredCondition->GetCondition();
		if (Query.IsValid() && !Query->Evaluate(Outcome)) return;
	}
	if (UItemAcquiredPayload* P = Cast<UItemAcquiredPayload>(Outcome.Payload))
	{
		UE_LOG(LogTemp, Log, TEXT("InventorySubsystem: Item acquired - Object: %s"),
			*P->ObjectId.ToString());
	}
	OnItemAcquired.Broadcast(Outcome);
}

void UInventorySubsystem::HandleInteractCommand(const FOutcomeEventBase& Outcome)
{
	if (const UInteractCommandPayload* P = Cast<UInteractCommandPayload>(Outcome.Payload))
	{
		const FGuid Id = P->ItemId;
		if (const FInventoryInteractItemRecord* Rec = RegisteredItems.Find(Id))
		{
			AActor* Owner = Rec->OwnerActor.Get();
			if (Owner)
			{
				ExecuteInteractCommandOnOwner(Id, Owner, P->Picker);
			}
		}
	}
}
/*
void UInventorySubsystem::HandleSetEnabled(const FOutcomeEventBase& Outcome)
{
	if (const UInteractSetEnabledPayload* P = Cast<UInteractSetEnabledPayload>(Outcome.Payload))
	{
		if (const FInventoryInteractItemRecord* Rec = RegisteredItems.Find(P->ItemId))
		{
			ExecuteSetEnabledOnOwner(P->ItemId, Rec->OwnerActor.Get(), P->bEnabled);
		}
	}
}
*/
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

void UInventorySubsystem::CollectSaveData(FSubsystemSaveData& OutData)
{
	// SubsystemName ставим всегда — чтобы ApplySaveData гарантированно вызвался на load
	OutData.SubsystemName = GetSaveSubsystemName();

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return;

	UAC_Inventory* Inventory = Pawn->FindComponentByClass<UAC_Inventory>();
	if (!Inventory || !Inventory->ContainerComponent) return;

	UAC_Container* Container = Inventory->ContainerComponent;

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// Items → массив объектов
	TArray<TSharedPtr<FJsonValue>> ItemsArray;
	ItemsArray.Reserve(Container->Items.Num());

	for (const FS_Item& Item : Container->Items)
	{
		TSharedPtr<FJsonObject> ItemJson = FJsonObjectConverter::UStructToJsonObject(Item);
		if (ItemJson.IsValid())
		{
			ItemsArray.Add(MakeShared<FJsonValueObject>(ItemJson));
		}
	}

	Root->SetArrayField(TEXT("Items"), ItemsArray);
	Root->SetNumberField(TEXT("TotalWeight"), Container->GetWeight());
	Root->SetNumberField(TEXT("TotalMoney"), Container->GetMoney());

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
	OutData.SerializedData = Output;

	UE_LOG(LogTemp, Log,
		TEXT("InventorySubsystem::CollectSaveData: %d items, weight=%.2f, money=%.2f"),
		Container->Items.Num(), Container->GetWeight(), Container->GetMoney());
}

void UInventorySubsystem::ApplySaveData(const FSubsystemSaveData& InData)
{
	IsLoadComplete = false;

	// Всегда чистим stale-состояние (могло остаться от floor transition)
	SavedInventorySnapshot.Reset();
	bHasSavedInventorySnapshot = false;

	if (InData.SerializedData.IsEmpty())
	{
		IsLoadComplete = true;
		return;
	}

	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(InData.SerializedData);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("InventorySubsystem::ApplySaveData: failed to parse JSON"));
		IsLoadComplete = true;
		return;
	}

	FInventorySnapshot Snapshot;

	// Items
	const TArray<TSharedPtr<FJsonValue>>* ItemsArray = nullptr;
	if (Root->TryGetArrayField(TEXT("Items"), ItemsArray))
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
				Snapshot.Items.Add(Item);
			}
		}
	}

	// TotalWeight / TotalMoney (double — универсально для UE5 TryGetNumberField)
	double TotalWeight = 0.0;
	if (Root->TryGetNumberField(TEXT("TotalWeight"), TotalWeight))
		Snapshot.TotalWeight = static_cast<float>(TotalWeight);

	double TotalMoney = 0.0;
	if (Root->TryGetNumberField(TEXT("TotalMoney"), TotalMoney))
		Snapshot.TotalMoney = static_cast<float>(TotalMoney);

	SavedInventorySnapshot = Snapshot;
	bHasSavedInventorySnapshot = true;

	IsLoadComplete = true;

	UE_LOG(LogTemp, Log,
		TEXT("InventorySubsystem::ApplySaveData: loaded %d items, weight=%.2f, money=%.2f (will be applied after map load)"),
		Snapshot.Items.Num(), Snapshot.TotalWeight, Snapshot.TotalMoney);
}
