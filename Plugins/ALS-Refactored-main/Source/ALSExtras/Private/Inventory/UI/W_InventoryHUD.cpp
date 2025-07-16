#include "Inventory/UI/W_InventoryHUD.h"
#include "Inventory/UI/W_Inventory.h"
#include "Inventory/AC_Container.h"
#include "Components/ScrollBox.h"
#include "Inventory/UI/W_ItemSlot.h"
#include "Inventory/UI/W_HowMuch.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Button.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UW_InventoryHUD::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			Input->BindAction(TakeAllAction, ETriggerEvent::Started, this, &UW_InventoryHUD::TakeAll);
			Input->BindAction(DropAllAction, ETriggerEvent::Started, this, &UW_InventoryHUD::DropAll);
		}
	}

	Button_TakeAll->OnPressed.AddDynamic(this, &UW_InventoryHUD::TakeAll);
	Button_DropAll->OnPressed.AddDynamic(this, &UW_InventoryHUD::DropAll);
}

void UW_InventoryHUD::Slot_OneClick(EnumInventoryType SlotInventoryType, UW_ItemSlot* SlotToInteract, FName KeyPressed)
{
	switch (SlotInventoryType)
	{
	case EnumInventoryType::Inventory:
	{
		if (KeyPressed == "Right" && AdditiveInventory == nullptr)
		{
			if (SlotToInteract->Item.Quantity > 1)
			{
				CheckHowMuch(MainInventory, nullptr, SlotToInteract, true);
			}
			else
			{
				RemoveFromSlotContainer(MainInventory, SlotToInteract, 1, true);
			}
		}

		if (KeyPressed == "Left" && AdditiveInventory)
		{
			switch (AdditiveInventory->InventoryType)
			{
			case EnumInventoryType::Chest:
			{
				if (SlotToInteract->Item.Quantity > 1)
				{
					CheckHowMuch(MainInventory, AdditiveInventory, SlotToInteract, false);
				}
				else
				{
					AddToSlotContainer(AdditiveInventory, SlotToInteract, 1);
					RemoveFromSlotContainer(MainInventory, SlotToInteract, 1, false);
				}
				break;
			}
			case EnumInventoryType::Corpse:
			{
				break;
			}
			case EnumInventoryType::Vendor:
			{
				if (SlotToInteract->Item.Quantity > 1)
				{
					CheckHowMuch(MainInventory, AdditiveInventory, SlotToInteract, false);
				}
				else
				{
					AddToSlotContainer(AdditiveInventory, SlotToInteract, 1);
					RemoveFromSlotContainer(MainInventory, SlotToInteract, 1, false);
				}
				break;
			}
			default:
				break;
			}
		}
		break;
	}
	case EnumInventoryType::Chest:
	case EnumInventoryType::Corpse:
	{
		if (KeyPressed == "Left")
		{
			if (SlotToInteract->Item.Quantity > 1)
			{
				CheckHowMuch(AdditiveInventory, MainInventory, SlotToInteract, false);
			}
			else
			{
				AddToSlotContainer(MainInventory, SlotToInteract, 1);
				RemoveFromSlotContainer(AdditiveInventory, SlotToInteract, 1, false);
			}
		}
		break;
	}
	case EnumInventoryType::Vendor:
	{
		if (KeyPressed == "Left")
		{
			if (SlotToInteract->Item.Quantity > 1)
			{
				CheckHowMuch(AdditiveInventory, MainInventory, SlotToInteract, false);
			}
			else
			{
				AddToSlotContainer(MainInventory, SlotToInteract, 1);
				RemoveFromSlotContainer(AdditiveInventory, SlotToInteract, 1, false);
			}
		}
		break;
	}
	default:
		break;
	}
}

void UW_InventoryHUD::CheckHowMuch(UW_Inventory* Inventory_From, UW_Inventory* Inventory_To, UW_ItemSlot* SlotToInteract, bool bShouldSpawn)
{
	if (HowMuchWidgetClass)
	{
		if (UW_HowMuch* HowMuchWidget = CreateWidget<UW_HowMuch>(GetWorld(), HowMuchWidgetClass))
		{
			bHowMuchIsOpen = true;
			HowMuchWidget->Slider_HowMuch->SetMaxValue(SlotToInteract->Item.Quantity);
			HowMuchWidget->TextBlock_Max->SetText(FText::AsNumber(SlotToInteract->Item.Quantity));
			HowMuchWidget->bShouldSpawn = bShouldSpawn;
			HowMuchWidget->InventoryHUDRef = this;
			HowMuchWidget->Inventory_FromRef = Inventory_From;
			HowMuchWidget->Inventory_ToRef = Inventory_To;
			HowMuchWidget->SlotRef = SlotToInteract;
			SizeBox_Confirmation_HowMuch->AddChild(HowMuchWidget);
		}
	}
}

void UW_InventoryHUD::AddToSlotContainer(UW_Inventory* Inventory_To, UW_ItemSlot* SlotToAdd, int32 QuantityToAdd)
{
	if (!Inventory_To)
	{
		return;
	}

	SlotToAdd->InventoryType = Inventory_To->InventoryType;

	FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(SlotToAdd->Item.Name, TEXT("Find row in datatable"));

	if (ItemData && ItemData->bCanStack)
	{
		TObjectPtr<UW_ItemSlot>* SlotToCorrect = Inventory_To->Slots.FindByPredicate([&](TObjectPtr<UW_ItemSlot> SlotToFind)
			{
				return SlotToFind->Item.Name == SlotToAdd->Item.Name;
			});

		if (SlotToCorrect)
		{
			(*SlotToCorrect)->Item.Quantity += QuantityToAdd;
			(*SlotToCorrect)->TextBlock_Quantity->SetText(FText::AsNumber((*SlotToCorrect)->Item.Quantity));
		}
		else
		{
			if (SlotToAdd->Item.Quantity != QuantityToAdd)
			{
				if (SlotWidgetClass)
				{
					if (TObjectPtr<UW_ItemSlot> ItemSlot = CreateWidget<UW_ItemSlot>(GetWorld(), SlotWidgetClass))
					{
						ItemSlot->Item.Name = SlotToAdd->Item.Name;
						ItemSlot->Item.Quantity = QuantityToAdd;
						ItemSlot->InventoryHUDRef = this;
						ItemSlot->InventoryType = Inventory_To->InventoryType;

						SlotToAdd = ItemSlot;
					}
				}
			}
			Inventory_To->Slots.Add(SlotToAdd);
		}
	}
	else
	{
		Inventory_To->Slots.Add(SlotToAdd);
	}
	Inventory_To->SlotsFilter(Inventory_To->CurrentTabType);
	Inventory_To->Container->AddToContainer(SlotToAdd->Item.Name, QuantityToAdd);
}

void UW_InventoryHUD::RemoveFromSlotContainer(UW_Inventory* Inventory_From, UW_ItemSlot* SlotToRemove, int32 QuantityToRemove, bool bShouldSpawn)
{
	if (SlotToRemove->Item.Quantity != QuantityToRemove)
	{
		int32 SlotIndex = Inventory_From->ScrollBox_Items->GetChildIndex(SlotToRemove);
		if (SlotIndex != INDEX_NONE)
		{
			UW_ItemSlot* SlotToChange = Cast<UW_ItemSlot>(Inventory_From->ScrollBox_Items->GetChildAt(SlotIndex));
			SlotToChange->Item.Quantity -= QuantityToRemove;
			SlotToChange->TextBlock_Quantity->SetText(FText::AsNumber(SlotToChange->Item.Quantity));
			SlotToChange->InventoryType = Inventory_From->InventoryType;
		}
	}
	else
	{
		Inventory_From->ScrollBox_Items->RemoveChild(SlotToRemove);
		Inventory_From->Slots.Remove(SlotToRemove);
	}
	Inventory_From->Container->RemoveFromContainer(SlotToRemove->Item.Name, QuantityToRemove, bShouldSpawn);
}

void UW_InventoryHUD::TakeAll()
{
	if (Button_TakeAll->IsVisible())
	{
		switch (InventoryType)
		{
		case EnumInventoryType::Inventory:
			break;
		case EnumInventoryType::Chest:
		case EnumInventoryType::Corpse:
		{
			if (UAC_Container* PlayerContainer = Cast<UAC_Container>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetComponentByClass(UAC_Container::StaticClass())))
			{
				for (const auto& Item : Container->Items)
				{
					PlayerContainer->AddToContainer(Item.Name, Item.Quantity);
				}
				Container->Items.Empty();
				Recreate();
			}
			break;
		}
		case EnumInventoryType::Vendor:
			break;
		default:
			break;
		}
	}
}

void UW_InventoryHUD::DropAll()
{
	if (Button_DropAll->IsVisible())
	{
		switch (InventoryType)
		{
		case EnumInventoryType::Inventory:
			break;
		case EnumInventoryType::Chest:
		{
			if (UAC_Container* PlayerContainer = Cast<UAC_Container>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)->GetComponentByClass(UAC_Container::StaticClass())))
			{
				for (const auto& Item : PlayerContainer->Items)
				{
					Container->AddToContainer(Item.Name, Item.Quantity);
				}
				PlayerContainer->Items.Empty();
				Recreate();
			}
			break;
		}
		case EnumInventoryType::Corpse:
			break;
		case EnumInventoryType::Vendor:
			break;
		default:
			break;
		}
	}
}

void UW_InventoryHUD::Recreate_Implementation()
{
}
