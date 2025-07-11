#include "Inventory/UI/W_Inventory.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Inventory/UI/W_ItemSlot.h"
#include "Inventory/AC_Container.h"

void UW_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

}

void UW_Inventory::SlotsFilter(EnumInventory SlotContainerType)
{
	ScrollBox_Items->ClearChildren();

	for (UW_ItemSlot* SlotToFilter : Slots)
	{
		if (SlotContainerType == EnumInventory::All || SlotContainerType == ItemDataTable->FindRow<FS_ItemData>(*(SlotToFilter->TextBlock_Name->GetText().ToString()), TEXT("Find row in datatable"))->Type)
		{
			ScrollBox_Items->AddChild(SlotToFilter);
		}
	}
}

void UW_Inventory::Items_Sort(EnumSortType SortType, EnumInventory SlotContainerType)
{
	if (PrevSort != SortType)
	{
		bIsDecreased = false;
	}

	switch (SortType)
	{
	case EnumSortType::A_Z:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return SlotA.TextBlock_Name->GetText().ToString() > SlotB.TextBlock_Name->GetText().ToString();
				});
			Container->Items_Sort(EnumSortType::A_Z, true);
		}
		else
		{
			bIsDecreased = true;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return SlotA.TextBlock_Name->GetText().ToString() < SlotB.TextBlock_Name->GetText().ToString();
				});
			Container->Items_Sort(EnumSortType::A_Z, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Damage:
		if (bIsDecreased)
		{

			bIsDecreased = false;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Damage->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Damage->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Damage, true);
		}
		else
		{
			bIsDecreased = true;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Damage->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Damage->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Damage, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Armour:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Armor->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Armor->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Armour, true);
		}
		else
		{
			bIsDecreased = true;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Armor->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Armor->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Armour, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Durability:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Durability->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Durability->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Durability, true);
		}
		else
		{
			bIsDecreased = true;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Durability->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Durability->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Durability, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Weight:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Weight->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Weight->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Weight, true);
		}
		else
		{
			bIsDecreased = true;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Weight->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Weight->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Weight, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Value:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Value->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Value->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Value, true);
		}
		else
		{
			bIsDecreased = true;
			Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
				{
					return FCString::Atof(*(SlotA.TextBlock_Value->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Value->GetText().ToString()));
				});
			Container->Items_Sort(EnumSortType::Value, false);
		}
		PrevSort = SortType;
		break;
	default:
		break;
	}

	SlotsFilter(SlotContainerType);
}
