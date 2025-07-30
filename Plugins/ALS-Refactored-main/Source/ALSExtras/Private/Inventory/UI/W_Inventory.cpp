#include "Inventory/UI/W_Inventory.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Inventory/UI/W_ItemSlot.h"
#include "Inventory/AC_Container.h"
#include "AlsCharacterExample.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Inventory/UI/W_FocusableButton.h"

void UW_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

	if (Container)
	{
		Container->OnArmourChanged.AddDynamic(this, &UW_Inventory::RefreshArmour);
		Container->OnWeightChanged.AddDynamic(this, &UW_Inventory::RefreshWeight);
		Container->OnMoneyChanged.AddDynamic(this, &UW_Inventory::RefreshMoney);
	}

	Button_Tab_All->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Tab_All_Pressed);
	Button_Tab_Weapon->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Tab_Weapon_Pressed);
	Button_Tab_Clothes->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Tab_Clothes_Pressed);
	Button_Tab_Consumables->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Tab_Consumables_Pressed);
	Button_Tab_Miscellaneous->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Tab_Miscellaneous_Pressed);
	Button_Tab_Others->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Tab_Others_Pressed);

	Button_Sort_AZ->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Sort_AZ_Pressed);
	Button_Sort_Damage->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Sort_Damage_Pressed);
	Button_Sort_Armour->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Sort_Armour_Pressed);
	Button_Sort_Durability->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Sort_Durability_Pressed);
	Button_Sort_Weight->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Sort_Weight_Pressed);
	Button_Sort_Value->OnPressed.AddDynamic(this, &UW_Inventory::Handle_Button_Sort_Value_Pressed);
}

void UW_Inventory::SlotsFilter(EnumInventory SlotContainerType)
{
	CurrentTabType = SlotContainerType;

	for (UWidget* ItemSlot : ScrollBox_Items->GetAllChildren())
	{
		if (UW_ItemSlot* SlotToFilter = Cast<UW_ItemSlot>(ItemSlot))
		{
			SlotToFilter->SetVisibility(ESlateVisibility::Collapsed);
			if (SlotContainerType == EnumInventory::All || SlotContainerType == ItemDataTable->FindRow<FS_ItemData>(SlotToFilter->Item.Name, TEXT("Find row in datatable"))->Type)
			{
				SlotToFilter->SetVisibility(ESlateVisibility::Visible);
			}
		}
	}
}

void UW_Inventory::RefreshArmour(float Armour)
{

}

void UW_Inventory::RefreshWeight(float Weight)
{
	if (AAlsCharacterExample* Character = Cast<AAlsCharacterExample>(Container->GetOwner()))
	{
		FString OutText(FString::Printf(TEXT("%.0f/%.1f"), Character->GetStrength(), Weight));

		if (Weight == FMath::Floor(Weight))
		{
			OutText = FString::Printf(TEXT("%.0f/%.0f"), Character->GetStrength(), Weight);
		}

		TextBlock_TotalWeight->SetText(FText::FromString(OutText));
	}
}

void UW_Inventory::RefreshMoney(float Money)
{
	FString OutText(FString::Printf(TEXT("%.2f"), Money));

	if (Money == FMath::Floor(Money))
	{
		OutText = FString::Printf(TEXT("%.0f"), Money);
	}

	TextBlock_TotalMoney->SetText(FText::FromString(OutText));
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
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return SlotA.TextBlock_Name->GetText().ToString() > SlotB.TextBlock_Name->GetText().ToString();
			//	});
			Container->Items_Sort(EnumSortType::A_Z, true);
		}
		else
		{
			bIsDecreased = true;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return SlotA.TextBlock_Name->GetText().ToString() < SlotB.TextBlock_Name->GetText().ToString();
			//	});
			Container->Items_Sort(EnumSortType::A_Z, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Damage:
		if (bIsDecreased)
		{

			bIsDecreased = false;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Damage->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Damage->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Damage, true);
		}
		else
		{
			bIsDecreased = true;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Damage->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Damage->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Damage, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Armour:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Armor->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Armor->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Armour, true);
		}
		else
		{
			bIsDecreased = true;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Armor->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Armor->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Armour, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Durability:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Durability->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Durability->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Durability, true);
		}
		else
		{
			bIsDecreased = true;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Durability->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Durability->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Durability, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Weight:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Weight->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Weight->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Weight, true);
		}
		else
		{
			bIsDecreased = true;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Weight->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Weight->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Weight, false);
		}
		PrevSort = SortType;
		break;
	case EnumSortType::Value:
		if (bIsDecreased)
		{
			bIsDecreased = false;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Value->GetText().ToString())) > FCString::Atof(*(SlotB.TextBlock_Value->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Value, true);
		}
		else
		{
			bIsDecreased = true;
			//ItemSlots.Sort([](const UW_ItemSlot& SlotA, const UW_ItemSlot& SlotB)
			//	{
			//		return FCString::Atof(*(SlotA.TextBlock_Value->GetText().ToString())) < FCString::Atof(*(SlotB.TextBlock_Value->GetText().ToString()));
			//	});
			Container->Items_Sort(EnumSortType::Value, false);
		}
		PrevSort = SortType;
		break;
	default:
		break;
	}

	RefreshAfterSort();

	SlotsFilter(SlotContainerType);
}

void UW_Inventory::Handle_Button_Tab_All_Pressed_Implementation()
{
}

void UW_Inventory::Handle_Button_Tab_Weapon_Pressed_Implementation()
{
}

void UW_Inventory::Handle_Button_Tab_Clothes_Pressed_Implementation()
{
}

void UW_Inventory::Handle_Button_Tab_Consumables_Pressed_Implementation()
{
}

void UW_Inventory::Handle_Button_Tab_Miscellaneous_Pressed_Implementation()
{
}

void UW_Inventory::Handle_Button_Tab_Others_Pressed_Implementation()
{
}

void UW_Inventory::Handle_Button_Sort_AZ_Pressed_Implementation()
{
	Items_Sort(EnumSortType::A_Z, ActiveTab);
}

void UW_Inventory::Handle_Button_Sort_Damage_Pressed_Implementation()
{
	Items_Sort(EnumSortType::Damage, ActiveTab);
}

void UW_Inventory::Handle_Button_Sort_Armour_Pressed_Implementation()
{
	Items_Sort(EnumSortType::Armour, ActiveTab);
}

void UW_Inventory::Handle_Button_Sort_Durability_Pressed_Implementation()
{
	Items_Sort(EnumSortType::Durability, ActiveTab);
}

void UW_Inventory::Handle_Button_Sort_Weight_Pressed_Implementation()
{
	Items_Sort(EnumSortType::Weight, ActiveTab);
}

void UW_Inventory::Handle_Button_Sort_Value_Pressed_Implementation()
{
	Items_Sort(EnumSortType::Value, ActiveTab);
}
