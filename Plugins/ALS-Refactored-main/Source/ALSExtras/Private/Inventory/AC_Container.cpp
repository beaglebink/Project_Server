#include "Inventory/AC_Container.h"
#include "Inventory/A_PickUp.h"
#include "Inventory/UI/W_ItemSlot.h"
#include "EventBusSystem/EventBusSubsystem.h"
#include "EventBusSystem/OutcomeEventBase.h"
#include "EventBusSystem/OutcomePayload.h"
#include "InventorySystem/InventoryPayloads.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"
#include <FloorAssignmentComponent.h>
#include "InventorySystem/InventorySubsystem.h"


UAC_Container::UAC_Container()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UAC_Container::IsPlayerContainer() const
{
	return Cast<AAlsCharacterExample>(GetOwner()) != nullptr;
}

FGuid UAC_Container::GetActorItemId() const
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		UFloorAssignmentComponent* FloorAssignmentComp = Owner->FindComponentByClass<UFloorAssignmentComponent>();
		if (FloorAssignmentComp && FloorAssignmentComp->ProtectedItemId.IsValid())
		{
			return FloorAssignmentComp->ProtectedItemId;
		}
	}
	return FGuid();
}

void UAC_Container::BeginPlay()
{
	Super::BeginPlay();

	CountContainerWeight();

	// Контейнер игрока — через Save/RestoreInventoryRequest в UInventorySubsystem.
	if (IsPlayerContainer()) return;

	const FGuid ItemId = GetActorItemId();
	if (!ItemId.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UAC_Container::BeginPlay: '%s' has no valid ItemId — not persisted"),
			*GetNameSafe(GetOwner()));
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;
	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	UInventorySubsystem* Sub = GI->GetSubsystem<UInventorySubsystem>();
	if (!Sub) return;

	// Живой реестр — CollectSaveData итерирует только его, без перебора всех акторов.
	Sub->RegisterLiveContainer(this);

	// Pull-восстановление: O(1) lookup.
	FInventorySnapshot Saved;
	if (Sub->GetContainerSnapshot(ItemId, Saved))
	{
		Items = Saved.Items;
		SetWeight(Saved.TotalWeight);
		SetMoney(Saved.TotalMoney);

		UE_LOG(LogTemp, Log,
			TEXT("UAC_Container::BeginPlay: Restored %s (%d items)"),
			*ItemId.ToString(), Items.Num());
	}
	// Если снапшота нет — оставляем дефолт BP, ничего не пишем.
}

void UAC_Container::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (IsPlayerContainer()) return;

	const FGuid ItemId = GetActorItemId();
	if (!ItemId.IsValid()) return;

	UWorld* World = GetWorld();
	if (!World) return;
	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	// 1) deregistration — CollectSaveData не читает умирающий компонент.
	if (UInventorySubsystem* Sub = GI->GetSubsystem<UInventorySubsystem>())
	{
		Sub->UnregisterLiveContainer(this);
	}

	// 2) публикуем актуальное состояние.
	if (UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>())
	{
		if (UInventoryContainerPayload* Payload = Bus->CreatePayload<UInventoryContainerPayload>())
		{
			Payload->ItemId = ItemId;
			Payload->Snapshot.Items = Items;
			Payload->Snapshot.TotalWeight = GetWeight();
			Payload->Snapshot.TotalMoney = GetMoney();

			FOutcomeEventBase Ev;
			Ev.OutcomeType = EOutcomeType::Inventory;
			Ev.OutcomeInventory = EOutcomeInventory::SaveContainerRequest;
			Ev.Payload = Payload;
			Bus->PublishOutcome(Ev);

			UE_LOG(LogTemp, Log,
				TEXT("UAC_Container::EndPlay: Saved %s (%d items)"),
				*ItemId.ToString(), Items.Num());
		}
	}
}


static void PublishInventoryChangeReport(UAC_Container* Self, EOutcomeInventory ChangeEvent, FName ItemName, int32 Quantity, float TradeCoeff, bool bShouldCount, int32 NewQuantity)
{
	if (!Self)
	{
		return;
	}

	AActor* Owner = Self->GetOwner();
	if (!Owner)
	{
		return;
	}

	// Только инвентарь персонажа игрока — сундуки/ящики игнорируем.
	if (!Cast<AAlsCharacterExample>(Owner))
	{
		return;
	}

	UWorld* World = Self->GetWorld();
	if (!World)
	{
		return;
	}
	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return;
	}

	UEventBusSubsystem* Bus = GI->GetSubsystem<UEventBusSubsystem>();
	if (!Bus)
	{
		return;
	}

	UInventoryChangePayload* Payload = Bus->CreatePayload<UInventoryChangePayload>();
	if (!Payload)
	{
		return;
	}

	Payload->Setup(ItemName, Quantity, TradeCoeff, bShouldCount, NewQuantity, Self->GetWeight(), Self->GetMoney());

	FOutcomeEventBase Ev;
	Ev.OutcomeType = EOutcomeType::Inventory;
	Ev.OutcomeInventory = ChangeEvent;
	Ev.Payload = Payload;
	Bus->PublishOutcome(Ev);
}

void UAC_Container::AddToContainer(FName Name, int32 Quantity, float TradeCoeff, bool bShouldCount)
{
	if (ItemDataTable)
	{
		FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"));

		if (!ItemData)
			return;

		// Итоговое количество предмета в слоте — нужно для пайлоада.
		int32 NewQuantity = 0;

		if (ItemData && ItemData->bCanStack)
		{
			FS_Item* ItemToAdd = Items.FindByPredicate([&](FS_Item& ArrayItem)
				{
					return ArrayItem.Name == Name;
				});

			if (ItemToAdd)
			{
				ItemToAdd->Quantity += Quantity;
				NewQuantity = ItemToAdd->Quantity;
			}
			else
			{
				Items.Add(FS_Item(Name, Quantity));
				NewQuantity = Quantity;
			}
		}
		else
		{
			Items.Add(FS_Item(Name, 1));
			NewQuantity = 1;
		}

		SetWeight(GetWeight() + ItemData->Weight * Quantity);

		if (bShouldCount)
		{
			SetMoney(GetMoney() - ItemData->Value * Quantity * TradeCoeff);
		}

		// Публикуем ПОСЛЕ всех расчётов (вес, деньги уже обновлены).
		PublishInventoryChangeReport(this, EOutcomeInventory::ItemAdd, Name, Quantity, TradeCoeff, bShouldCount, NewQuantity);
	}
}

void UAC_Container::RemoveFromContainer(int32 ItemIndex, int32 Quantity, float TradeCoeff, bool bShouldCount, bool bShouldSpawn)
{
	if (!Items.IsValidIndex(ItemIndex))
	{
		return;
	}

	FName Name = Items[ItemIndex].Name;

	if (bShouldSpawn)
	{
		ItemsToSpawn.FindOrAdd(Name) += Quantity;

		if (!RemoveItemsHandle.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(RemoveItemsHandle, [this]()
				{
					if (ItemsToSpawn.IsEmpty())
					{
						GetWorld()->GetTimerManager().ClearTimer(RemoveItemsHandle);
					}

					TArray<FName> KeysToRemove;

					for (auto& [Key, Value] : ItemsToSpawn)
					{
						for (size_t i = 0; i < FMath::Min(Value, 1); ++i)
						{
							if (SpawnRemovedItem(Key))
							{
								--ItemsToSpawn[Key];
							}
						}
						if (ItemsToSpawn[Key] <= 0)
						{
							KeysToRemove.Add(Key);
						}
					}

					for (FName Key : KeysToRemove)
					{
						ItemsToSpawn.Remove(Key);
					}
				}, 0.01f, true, 0);
		}
	}

	// Итоговое количество в слоте — нужно для пайлоада.
	int32 NewQuantity = 0;

	if (Items[ItemIndex].Quantity > Quantity)
	{
		Items[ItemIndex].Quantity -= Quantity;
		NewQuantity = Items[ItemIndex].Quantity;
	}
	else
	{
		Items.RemoveAt(ItemIndex);
		NewQuantity = 0;
	}

	if (ItemDataTable)
	{
		FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"));

		SetWeight(GetWeight() - ItemData->Weight * Quantity);

		if (bShouldCount)
		{
			SetMoney(GetMoney() + ItemData->Value * Quantity * TradeCoeff);
		}
	}

	// Публикуем ПОСЛЕ всех расчётов.
	PublishInventoryChangeReport(this, EOutcomeInventory::ItemRemove, Name, Quantity, TradeCoeff, bShouldCount, NewQuantity);
}

bool UAC_Container::FindItemByName(FName Name, int32& OutItemIndex, int32& OutQuantity) const
{
	OutItemIndex = INDEX_NONE;
	OutQuantity = 0;

	const int32 Index = Items.IndexOfByPredicate(
		[&Name](const FS_Item& Item) { return Item.Name == Name; });

	if (Index == INDEX_NONE)
	{
		return false;
	}

	OutItemIndex = Index;
	OutQuantity = Items[Index].Quantity;
	return true;
}

bool UAC_Container::SetItemQuantityByName(FName Name, int32 NewQuantity)
{
	if (NewQuantity < 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UAC_Container::SetItemQuantityByName: negative quantity (%d) for '%s'"),
			NewQuantity, *Name.ToString());
		return false;
	}

	int32 Index = INDEX_NONE;
	int32 OldQuantity = 0;
	const bool bExists = FindItemByName(Name, Index, OldQuantity);

	// Идемпотентный вызов: запрошенное состояние уже достигнуто.
	//   - предмета нет, просят 0     → уже так;
	//   - предмет есть, просят то же → уже так.
	if ((!bExists && NewQuantity == 0) || (bExists && OldQuantity == NewQuantity))
	{
		return true;
	}

	// Данные для расчёта веса. Если предмета нет в DataTable — создаём/обновляем
	// «как есть», вес не меняется, но предупреждаем (обычно это баг в DataTable).
	const FS_ItemData* ItemData = ItemDataTable
		? ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"))
		: nullptr;

	if (!ItemData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UAC_Container::SetItemQuantityByName: item '%s' not found in ItemDataTable — weight is not affected"),
			*Name.ToString());
	}

	// Дельта веса. Для нового стека OldQuantity == 0 → дельта = Weight * NewQuantity.
	float WeightDelta = 0.0f;
	if (ItemData)
	{
		WeightDelta = ItemData->Weight * static_cast<float>(NewQuantity - OldQuantity);
	}

	// Применяем изменение к массиву.
	if (NewQuantity == 0)
	{
		// Сюда попадаем только при bExists == true (иначе сработал бы идемпотентный return).
		Items.RemoveAt(Index);
	}
	else if (bExists)
	{
		Items[Index].Quantity = NewQuantity;
	}
	else
	{
		// Создаём новый стек. Даже для не-стакуемых предметов Quantity может быть > 1 —
		// это ответственность вызывающего.
		Items.Add(FS_Item(Name, NewQuantity));
	}

	// Обновляем вес через дельту (SetWeight broadcast'ит OnWeightChanged).
	if (!FMath::IsNearlyZero(WeightDelta))
	{
		SetWeight(GetWeight() + WeightDelta);
	}

	// ── Публикуем отчёт об изменении (только для игрока — фильтр внутри) ──
	// Delta > 0: ItemAdd с Quantity = Delta, NewQuantity = итоговое.
	// Delta < 0: ItemRemove с Quantity = |Delta|, NewQuantity = итоговое.
	// "New stack" трактуется как ItemAdd(Quantity = NewQuantity).
	const int32 Delta = NewQuantity - OldQuantity;
	if (Delta > 0)
	{
		PublishInventoryChangeReport(
			this, EOutcomeInventory::ItemAdd,
			Name, Delta, /*TradeCoeff*/ 1.0f, /*bShouldCount*/ false,
			/*NewQuantity*/ NewQuantity);
	}
	else if (Delta < 0)
	{
		PublishInventoryChangeReport(
			this, EOutcomeInventory::ItemRemove,
			Name, -Delta, /*TradeCoeff*/ 1.0f, /*bShouldCount*/ false,
			/*NewQuantity*/ NewQuantity);
	}

	const FString OldQtyStr = bExists ? FString::FromInt(OldQuantity) : TEXT("<new>");
	UE_LOG(LogTemp, Log,
		TEXT("UAC_Container::SetItemQuantityByName: '%s' %s -> %d (weight delta %.2f)"),
		*Name.ToString(), *OldQtyStr, NewQuantity, WeightDelta);

	return true;
}

bool UAC_Container::SpawnRemovedItem(FName Name)
{
	FVector Origin = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 200.0f;
	float Radius = 100.0f;
	FVector RandomOffset = FMath::VRand() * FMath::FRandRange(0.0f, Radius);
	FVector SpawnLocation = Origin + RandomOffset;

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SpawnLocation);
	SpawnTransform.SetRotation(FQuat::Identity);
	SpawnTransform.SetScale3D(FVector(1.f));
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	AA_PickUp* PickUpActor = GetWorld()->SpawnActor<AA_PickUp>(AA_PickUp::StaticClass(), SpawnTransform, SpawnParams);

	if (!PickUpActor || !ItemDataTable)
	{
		return false;
	}

	PickUpActor->Name = Name;
	PickUpActor->StaticMeshComp->SetStaticMesh(ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"))->StaticMesh);
	PickUpActor->StaticMeshComp->SetMobility(EComponentMobility::Movable);
	PickUpActor->StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PickUpActor->StaticMeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
	PickUpActor->StaticMeshComp->SetSimulatePhysics(true);
	PickUpActor->Sound = PickUpSound;

	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound, PickUpActor->GetActorLocation());
	}
	return true;
}

bool UAC_Container::SetItemMarking(UW_ItemSlot* SlotToBeMarked, EItemMarking NewMarking)
{
	int32 ItemIndex = Items.IndexOfByKey(SlotToBeMarked->Item);
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	if (Items[ItemIndex].Marking != EItemMarking::CantBeUsed)
	{
		Items[ItemIndex].Marking = NewMarking;
		SlotToBeMarked->SetMarkOnSelected(NewMarking);
		return true;
	}

	return false;
}

void UAC_Container::Items_Sort(EnumSortType SortType, bool bIsDecreasing)
{
	if (!ItemDataTable)
	{
		return;
	}

	switch (SortType)
	{
	case EnumSortType::A_Z:
	{
		if (bIsDecreasing)
		{
			Items.Sort([](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return ItemA.Name.ToString() > ItemB.Name.ToString();
				});
		}
		else
		{
			Items.Sort([](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return ItemA.Name.ToString() < ItemB.Name.ToString();
				});
		}
		break;
	}
	case EnumSortType::Damage:
	{
		if (bIsDecreasing)
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Damage > ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Damage;
				});
		}
		else
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Damage < ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Damage;
				});
		}
		break;
	}
	case EnumSortType::Armour:
	{
		if (bIsDecreasing)
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Armor > ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Armor;
				});
		}
		else
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Armor < ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Armor;
				});
		}
		break;
	}
	case EnumSortType::Durability:
	{
		if (bIsDecreasing)
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Durability > ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Durability;
				});
		}
		else
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Durability < ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Durability;
				});
		}
		break;
	}
	case EnumSortType::Weight:
	{
		if (bIsDecreasing)
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Weight > ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Weight;
				});
		}
		else
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Weight < ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Weight;
				});
		}
		break;
	}
	case EnumSortType::Value:
	{
		if (bIsDecreasing)
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Value > ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Value;
				});
		}
		else
		{
			Items.Sort([&](const FS_Item& ItemA, const FS_Item& ItemB)
				{
					return  ItemDataTable->FindRow<FS_ItemData>(ItemA.Name, TEXT("Find row in datatable"))->Value < ItemDataTable->FindRow<FS_ItemData>(ItemB.Name, TEXT("Find row in datatable"))->Value;
				});
		}
		break;
	}
	default:
		break;
	}
}

float UAC_Container::GetWeight()
{
	return TotalWeight;
}

float UAC_Container::GetMoney()
{
	return TotalMoney;
}

void UAC_Container::SetWeight(float NewWeight)
{
	TotalWeight = NewWeight;
	OnWeightChanged.Broadcast(NewWeight);
}

void UAC_Container::SetMoney(float NewValue)
{
	TotalMoney = NewValue;
	OnMoneyChanged.Broadcast(NewValue);
}

void UAC_Container::CountContainerWeight()
{
	if (ItemDataTable)
	{
		for (const auto& Item : Items)
		{
			if (FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(Item.Name, TEXT("Find row in datatable")))
			{
				TotalWeight += ItemData->Weight * Item.Quantity;
			}
		}
	}
}

int32 UAC_Container::GetItemIndexByTag(FGameplayTag ItemTag) const
{
	FString TagString = ItemTag.GetTagName().ToString();
	FString LastPart;
	TagString.Split(TEXT("."), nullptr, &LastPart, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	return Items.IndexOfByPredicate([&](const FS_Item& Item)
		{
			return Item.Name == FName(*LastPart);
		});
}
