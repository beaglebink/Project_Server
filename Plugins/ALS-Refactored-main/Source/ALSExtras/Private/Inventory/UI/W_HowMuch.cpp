#include "Inventory/UI/W_HowMuch.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EnhancedInputComponent.h"

void UW_HowMuch::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			Input->BindAction(SurfAction, ETriggerEvent::Triggered, this, &UW_HowMuch::SurfSlider);
			Input->BindAction(UseAction, ETriggerEvent::Started, this, &UW_HowMuch::ConfirmSlider);
			Input->BindAction(EscapeAction, ETriggerEvent::Started, this, &UW_HowMuch::EscapeWidget);
		}
	}

	Button_Yes->OnReleased.AddDynamic(this, &UW_HowMuch::ConfirmSlider);
	Button_No->OnReleased.AddDynamic(this, &UW_HowMuch::EscapeWidget);
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
	InventoryHUDRef->AddToSlotContainer(Inventory_ToRef, SlotRef,FMath::RoundToInt32(Slider_HowMuch->GetValue()));
	InventoryHUDRef->RemoveFromSlotContainer(Inventory_FromRef, SlotRef, FMath::RoundToInt32(Slider_HowMuch->GetValue()), bShouldSpawn);
	RemoveFromParent();
	MarkAsGarbage();
}

void UW_HowMuch::EscapeWidget()
{
	RemoveFromParent();
	MarkAsGarbage();
}
