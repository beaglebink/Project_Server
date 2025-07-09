#include "Inventory/AC_Container.h"

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
}

void UAC_Container::RemoveFromContainer(FName Name, int32 Quantity)
{
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
}

void UAC_Container::A_Z_Sort(bool bIsDecreasing)
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
}

void UAC_Container::Damage_Sort(bool bIsDecreasing)
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
}

void UAC_Container::Armor_Sort(bool bIsDecreasing)
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
}

void UAC_Container::Durability_Sort(bool bIsDecreasing)
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
}

void UAC_Container::Weight_Sort(bool bIsDecreasing)
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
}

void UAC_Container::Value_Sort(bool bIsDecreasing)
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
}
