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
