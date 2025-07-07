#include "Inventory/UI/W_SlotContainer.h"
#include "Inventory/UI/W_ItemSlot.h"
#include "Components/TextBlock.h"

void UW_SlotContainer::NativeConstruct()
{
	Super::NativeConstruct();

}

void UW_SlotContainer::A_Z_Sort()
{
	if (bIs_A_Z_Sort)
	{
		bIs_A_Z_Sort = false;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return SlotA.TextBlock_Name->GetText().ToString() > SlotB.TextBlock_Name->GetText().ToString();
			});
	}
	else
	{
		bIs_A_Z_Sort = true;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return SlotA.TextBlock_Name->GetText().ToString() < SlotB.TextBlock_Name->GetText().ToString();
			});
	}
}

void UW_SlotContainer::Damage_Sort()
{
	if (bIs_Damage_Sort)
	{

		bIs_Damage_Sort = false;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Damage->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Damage->GetText().ToString()));
			});
	}
	else
	{
		bIs_Damage_Sort = true;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Damage->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Damage->GetText().ToString()));
			});
	}
}

void UW_SlotContainer::Armor_Sort()
{
	if (bIs_Armor_Sort)
	{
		bIs_Armor_Sort = false;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Armor->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Armor->GetText().ToString()));
			});
	}
	else
	{
		bIs_Armor_Sort = true;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Armor->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Armor->GetText().ToString()));
			});
	}
}

void UW_SlotContainer::Durability_Sort()
{
	if (bIs_Durability_Sort)
	{
		bIs_Durability_Sort = false;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Durability->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Durability->GetText().ToString()));
			});
	}
	else
	{
		bIs_Durability_Sort = true;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Durability->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Durability->GetText().ToString()));
			});
	}
}

void UW_SlotContainer::Weight_Sort()
{
	if (bIs_Weight_Sort)
	{
		bIs_Weight_Sort = false;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Weight->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Weight->GetText().ToString()));
			});
	}
	else
	{
		bIs_Weight_Sort = true;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Weight->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Weight->GetText().ToString()));
			});
	}
}

void UW_SlotContainer::Value_Sort()
{
	if (bIs_Value_Sort)
	{
		bIs_Value_Sort = false;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Value->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Value->GetText().ToString()));
			});
	}
	else
	{
		bIs_Value_Sort = true;
		Slots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			{
				return FCString::Atof(*(SlotA.TextBlock_Value->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Value->GetText().ToString()));
			});
	}
}
