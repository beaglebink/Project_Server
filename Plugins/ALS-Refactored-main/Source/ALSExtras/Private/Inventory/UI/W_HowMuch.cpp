#include "Inventory/UI/W_HowMuch.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UW_HowMuch::NativeConstruct()
{
	Super::NativeConstruct();
}

void UW_HowMuch::NativeDestruct()
{
	Super::NativeDestruct();
	InventoryHUDRef->bHowMuchIsOpen = false;
}
