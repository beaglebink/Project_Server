#include "Inventory/UI/W_InventoryHUD.h"
#include "Inventory/UI/W_Inventory.h"
#include "Inventory/AC_Container.h"
#include "Components/ScrollBox.h"
#include "Inventory/UI/W_ItemSlot.h"
#include "Inventory/UI/W_HowMuch.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void UW_InventoryHUD::NativeConstruct()
{
	Super::NativeConstruct();
}

void UW_InventoryHUD::MoveSlot(EnumInventoryType SlotInventoryType, UW_ItemSlot* SlotToMove, FName KeyPressed)
{
	switch (SlotInventoryType)
	{
	case EnumInventoryType::Inventory:
	{
		if (AdditiveInventory == nullptr)
		{
			if (KeyPressed == "Right")
			{
				RemoveFromSlotContainer(MainInventory, SlotToMove, true);
			}
		}
		else
		{

		}
		break;
	}
	case EnumInventoryType::Chest:
		break;
	case EnumInventoryType::Corpse:
		break;
	case EnumInventoryType::Vendor:
		break;
	default:
		break;
	}
}

void UW_InventoryHUD::AddToSlotContainer(UW_Inventory* Inventory, UW_ItemSlot* SlotToAdd)
{
}

void UW_InventoryHUD::RemoveFromSlotContainer(UW_Inventory* Inventory, UW_ItemSlot* SlotToRemove, bool bShouldSpawn)
{
	if (SlotToRemove->Item.Quantity > 1)
	{
		if (HowMuchWidgetClass)
		{
			if (UW_HowMuch* HowMuchWidget = CreateWidget<UW_HowMuch>(GetWorld(), HowMuchWidgetClass))
			{
				HowMuchWidget->Slider_HowMuch->SetMaxValue(SlotToRemove->Item.Quantity);
				HowMuchWidget->TextBlock_Max->SetText(FText::AsNumber(SlotToRemove->Item.Quantity));
				HowMuchWidget->bShouldSpawn = bShouldSpawn;
				HowMuchWidget->InventoryHUDRef = this;
				HowMuchWidget->InventoryRef = Inventory;
				HowMuchWidget->SlotRef = SlotToRemove;
				SizeBox_Confirmation_HowMuch->AddChild(HowMuchWidget);
			}
		}
	}
	else
	{
		RemoveSlotCertainQuantity(Inventory, SlotToRemove, 1, bShouldSpawn);
	}
}

void UW_InventoryHUD::RemoveSlotCertainQuantity(UW_Inventory* Inventory, UW_ItemSlot* SlotToRemove, int32 QuantityToRemove, bool bShouldSpawn)
{
	if (SlotToRemove->Item.Quantity != QuantityToRemove)
	{
		int32 SlotIndex = Inventory->ScrollBox_Items->GetChildIndex(SlotToRemove);
		if (SlotIndex != INDEX_NONE)
		{
			UW_ItemSlot* SlotToChange = Cast<UW_ItemSlot>(Inventory->ScrollBox_Items->GetChildAt(SlotIndex));
			SlotToChange->Item.Quantity -= QuantityToRemove;
			SlotToChange->TextBlock_Quantity->SetText(FText::AsNumber(SlotToChange->Item.Quantity));
		}
	}
	else
	{
		Inventory->ScrollBox_Items->RemoveChild(SlotToRemove);
		Inventory->Slots.Remove(SlotToRemove);
	}
	Inventory->Container->RemoveFromContainer(SlotToRemove->Item.Name, QuantityToRemove, bShouldSpawn);
}

void UW_InventoryHUD::Recreate_Implementation()
{
}
