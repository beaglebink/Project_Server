#include "Inventory/UI/W_HowMuch.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Inventory/UI/W_FocusableButton.h"
#include "EnhancedInputComponent.h"
#include "Inventory/UI/W_Inventory.h"
#include "Inventory/AC_Container.h"
#include "Inventory/UI/W_ItemSlot.h"

void UW_HowMuch::NativeConstruct()
{
	Super::NativeConstruct();

	bIsFocusable = true;
	Button_Yes->SetKeyboardFocus();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			Input->BindAction(SurfAction, ETriggerEvent::Triggered, this, &UW_HowMuch::SurfSlider);
		}
	}

	Button_Yes->OnPressed.AddDynamic(this, &UW_HowMuch::ConfirmSlider);
	Button_No->OnPressed.AddDynamic(this, &UW_HowMuch::EscapeWidget);
}

void UW_HowMuch::NativeDestruct()
{
	Super::NativeDestruct();

	InventoryHUDRef->bHowMuchIsOpen = false;
}

void UW_HowMuch::SurfSlider(const FInputActionValue& Value)
{
	FVector2D Direction = Value.Get<FVector2D>();

	Slider_HowMuch->SetValue(FMath::Clamp(Slider_HowMuch->GetValue() + Direction.X + Direction.Y * 10, 0.0f, Slider_HowMuch->GetMaxValue()));
}

void UW_HowMuch::ConfirmSlider()
{
	SlotRef->SetKeyboardFocus();
	if (FMath::RoundToInt32(Slider_HowMuch->GetValue()) != 0)
	{
		InventoryHUDRef->AddToSlotContainer(Inventory_ToRef, SlotRef, FMath::RoundToInt32(Slider_HowMuch->GetValue()), bShouldCount);
		InventoryHUDRef->RemoveFromSlotContainer(Inventory_FromRef, SlotRef, FMath::RoundToInt32(Slider_HowMuch->GetValue()), bShouldCount, bShouldSpawn);
	}
	RemoveFromParent();
	MarkAsGarbage();
}

void UW_HowMuch::EscapeWidget()
{
	SlotRef->SetKeyboardFocus();
	RemoveFromParent();
	MarkAsGarbage();
}
