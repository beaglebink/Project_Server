#include "Inventory/AC_Container.h"
#include "Inventory/A_PickUp.h"
#include "Kismet/GameplayStatics.h"

UAC_Container::UAC_Container()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAC_Container::AddToContainer(FName Name, int32 Quantity)
{
	FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"));

	if (ItemData && ItemData->bCanStack)
	{
		FS_Item* ItemToAdd = Items.FindByPredicate([&](FS_Item& ArrayItem)
			{
				return ArrayItem.Name == Name;
			});

		if (ItemToAdd)
		{
			ItemToAdd->Quantity += Quantity;
		}
		else
		{
			Items.Add(FS_Item(Name, Quantity));
		}
	}
	else
	{
		Items.Add(FS_Item(Name, 1));
	}

	TotalWeight += ItemData->Weight * Quantity;
	OnWeightChanged.Broadcast(TotalWeight);
}

void UAC_Container::RemoveFromContainer(FName Name, int32 Quantity, bool bShouldSpawn)
{
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
	int32 IndexToRemove = Items.IndexOfByPredicate([&](const FS_Item& ArrayItem)
		{
			return ArrayItem.Name == Name;
		});

	if (IndexToRemove != INDEX_NONE)
	{
		if (Items[IndexToRemove].Quantity > Quantity)
		{
			Items[IndexToRemove].Quantity -= Quantity;
		}
		else
		{
			Items.RemoveAt(IndexToRemove);
		}
	}

	FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"));

	TotalWeight -= ItemData->Weight * Quantity;
	OnWeightChanged.Broadcast(TotalWeight);
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

	AA_PickUp* TempActor = GetWorld()->SpawnActorDeferred<AA_PickUp>(AA_PickUp::StaticClass(), SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);

	if (!TempActor)
	{
		return false;
	}

	TempActor->Name = Name;
	TempActor->StaticMeshComp->SetStaticMesh(ItemDataTable->FindRow<FS_ItemData>(Name, TEXT("Find row in datatable"))->StaticMesh);
	TempActor->StaticMeshComp->SetMobility(EComponentMobility::Movable);
	TempActor->StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TempActor->StaticMeshComp->SetCollisionProfileName(TEXT("PhysicsActor"));
	TempActor->StaticMeshComp->SetSimulatePhysics(true);
	TempActor->Sound = PickUpSound;

	AA_PickUp* FinalActor = Cast<AA_PickUp>(UGameplayStatics::FinishSpawningActor(TempActor, SpawnTransform));

	if (!FinalActor)
	{
		return false;
	}

	if (SpawnSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), SpawnSound, FinalActor->GetActorLocation());
	}
	return true;
}

void UAC_Container::Items_Sort(EnumSortType SortType, bool bIsDecreasing)
{
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
