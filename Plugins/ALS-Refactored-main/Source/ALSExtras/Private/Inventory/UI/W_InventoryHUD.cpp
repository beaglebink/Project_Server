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
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Inventory/UI/W_NotEnough.h"

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

	if (NotEnoughWidgetClass)
	{
		NotEnoughWidget = CreateWidget<UW_NotEnough>(GetWorld(), NotEnoughWidgetClass);
		if (NotEnoughWidget)
		{
			UCanvasPanelSlot* BoxSlot = CanvasPanel_HowMuch_NotEnough->AddChildToCanvas(NotEnoughWidget);
			if (BoxSlot)
			{
				BoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				BoxSlot->SetPosition(FVector2D(0.f, 0.f));
				BoxSlot->SetSize(FVector2D(300.f, 150.f));
				BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				BoxSlot->SetZOrder(1);
			}
		}
	}

	FSlateApplication::Get().OnFocusChanging().AddUObject(this, &UW_InventoryHUD::OnFocusChangingSlate);
	OnFocusChangingWidget.AddDynamic(this, &UW_InventoryHUD::OnFocusChanging);
}

void UW_InventoryHUD::Slot_OneClick(EnumInventoryType SlotInventoryType, UW_ItemSlot* SlotToInteract, FName KeyPressed)
{
	bIsMoneyEnough = true;

	switch (SlotInventoryType)
	{
	case EnumInventoryType::Inventory:
	{
		if (KeyPressed == "Right" && AdditiveInventory == nullptr)
		{
			if (SlotToInteract->Item.Quantity > 1)
			{
				CheckHowMuch(MainInventory, nullptr, SlotToInteract, false, true);
			}
			else
			{
				RemoveFromSlotContainer(MainInventory, SlotToInteract, 1, false, true);
			}
		}

		if (KeyPressed == "Left" && AdditiveInventory)
		{
			switch (AdditiveInventory->InventoryType)
			{
			case EnumInventoryType::Chest:
			{
				CurrentTradeCoeff = 1.0f;

				if (SlotToInteract->Item.Quantity > 1)
				{
					CheckHowMuch(MainInventory, AdditiveInventory, SlotToInteract, false, false);
				}
				else
				{
					AddToSlotContainer(AdditiveInventory, SlotToInteract, 1, false);
					RemoveFromSlotContainer(MainInventory, SlotToInteract, 1, false, false);
				}
				break;
			}
			case EnumInventoryType::Corpse:
			{
				break;
			}
			case EnumInventoryType::Vendor:
			{
				CurrentTradeCoeff = 1 / AdditiveInventory->Container->TradeCoefficient;

				if (SlotToInteract->Item.Quantity > 1)
				{
					CheckHowMuch(MainInventory, AdditiveInventory, SlotToInteract, true, false);
				}
				else
				{
					AddToSlotContainer(AdditiveInventory, SlotToInteract, 1, true);
					RemoveFromSlotContainer(MainInventory, SlotToInteract, 1, true, false);
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
			CurrentTradeCoeff = 1.0f;

			if (SlotToInteract->Item.Quantity > 1)
			{
				CheckHowMuch(AdditiveInventory, MainInventory, SlotToInteract, false, false);
			}
			else
			{
				AddToSlotContainer(MainInventory, SlotToInteract, 1, false);
				RemoveFromSlotContainer(AdditiveInventory, SlotToInteract, 1, false, false);
			}
		}
		break;
	}
	case EnumInventoryType::Vendor:
	{
		if (KeyPressed == "Left")
		{
			CurrentTradeCoeff = AdditiveInventory->Container->TradeCoefficient;

			if (SlotToInteract->Item.Quantity > 1)
			{
				CheckHowMuch(AdditiveInventory, MainInventory, SlotToInteract, true, false);
			}
			else
			{
				AddToSlotContainer(MainInventory, SlotToInteract, 1, true);
				RemoveFromSlotContainer(AdditiveInventory, SlotToInteract, 1, true, false);
			}
		}
		break;
	}
	default:
		break;
	}
}

void UW_InventoryHUD::CheckHowMuch(UW_Inventory* Inventory_From, UW_Inventory* Inventory_To, UW_ItemSlot* SlotToInteract, bool bShouldCount, bool bShouldSpawn)
{
	if (HowMuchWidgetClass)
	{
		if (UW_HowMuch* HowMuchWidget = CreateWidget<UW_HowMuch>(GetWorld(), HowMuchWidgetClass))
		{
			bHowMuchIsOpen = true;
			HowMuchWidget->Slider_HowMuch->SetMaxValue(SlotToInteract->Item.Quantity);
			HowMuchWidget->TextBlock_Max->SetText(FText::AsNumber(SlotToInteract->Item.Quantity));
			HowMuchWidget->bShouldCount = bShouldCount;
			HowMuchWidget->bShouldSpawn = bShouldSpawn;
			HowMuchWidget->InventoryHUDRef = this;
			HowMuchWidget->Inventory_FromRef = Inventory_From;
			HowMuchWidget->Inventory_ToRef = Inventory_To;
			HowMuchWidget->SlotRef = SlotToInteract;

			UCanvasPanelSlot* BoxSlot = CanvasPanel_HowMuch_NotEnough->AddChildToCanvas(HowMuchWidget);
			if (BoxSlot)
			{
				BoxSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				BoxSlot->SetPosition(FVector2D(0.f, 0.f));
				BoxSlot->SetSize(FVector2D(300.f, 150.f));
				BoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
				BoxSlot->SetZOrder(0);
			}
		}
	}
}

void UW_InventoryHUD::AddToSlotContainer(UW_Inventory* Inventory_To, UW_ItemSlot* SlotToAdd, int32 QuantityToAdd, bool bShouldCount)
{
	if (!Inventory_To)
	{
		return;
	}

	FS_ItemData* ItemData = ItemDataTable->FindRow<FS_ItemData>(SlotToAdd->Item.Name, TEXT("Find row in datatable"));

	if (bShouldCount && ItemData && Inventory_To->Container->GetMoney() - ItemData->Value * QuantityToAdd * CurrentTradeCoeff < 0.0f)
	{
		NotEnoughWidget->PlayAppearing();
		bIsMoneyEnough = false;
		return;
	}

	SlotToAdd->InventoryType = Inventory_To->InventoryType;


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
			SlotToAdd->TextBlock_Value->SetText(SlotToAdd->FormatFloatFixed(ItemData->Value / CurrentTradeCoeff, 2));
			Inventory_To->Slots.Add(SlotToAdd);
		}
	}
	else
	{
		SlotToAdd->TextBlock_Value->SetText(SlotToAdd->FormatFloatFixed(ItemData->Value / CurrentTradeCoeff, 2));
		Inventory_To->Slots.Add(SlotToAdd);
	}
	Inventory_To->SlotsFilter(Inventory_To->CurrentTabType);
	Inventory_To->Container->AddToContainer(SlotToAdd->Item.Name, QuantityToAdd, CurrentTradeCoeff, bShouldCount);
}

void UW_InventoryHUD::RemoveFromSlotContainer(UW_Inventory* Inventory_From, UW_ItemSlot* SlotToRemove, int32 QuantityToRemove, bool bShouldCount, bool bShouldSpawn)
{
	if (!bIsMoneyEnough)
	{
		return;
	}

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
	Inventory_From->Container->RemoveFromContainer(SlotToRemove->Item.Name, QuantityToRemove, CurrentTradeCoeff, bShouldCount, bShouldSpawn);
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
					PlayerContainer->AddToContainer(Item.Name, Item.Quantity, 1.0f, false);
				}
				Container->Items.Empty();
				Container->SetWeight(0.0f);
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
					Container->AddToContainer(Item.Name, Item.Quantity, 1.0f, false);
				}
				PlayerContainer->Items.Empty();
				PlayerContainer->SetWeight(0.0f);
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

void UW_InventoryHUD::OnFocusChangingSlate(const FFocusEvent& FocusEvent, const FWeakWidgetPath& Path, const TSharedPtr<SWidget>& Widget, const FWidgetPath& WidgetPath, const TSharedPtr<SWidget>& SharedWidget)
{
	OnFocusChangingWidget.Broadcast();
}

void UW_InventoryHUD::OnFocusChanging_Implementation()
{
}
