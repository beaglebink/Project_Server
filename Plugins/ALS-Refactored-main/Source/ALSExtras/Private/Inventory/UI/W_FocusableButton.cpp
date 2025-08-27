#include "Inventory/UI/W_FocusableButton.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void UW_FocusableButton::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true); 
	//bIsFocusable = true; //'UUserWidget::bIsFocusable': Direct access to bIsFocusable is deprecated. Please use the getter. Note that this property is only set at construction and is not modifiable at runtime. Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.

	Button->OnClicked.AddDynamic(this, &UW_FocusableButton::HandleClicked);
	Button->OnPressed.AddDynamic(this, &UW_FocusableButton::HandlePressed);
	Button->OnReleased.AddDynamic(this, &UW_FocusableButton::HandleReleased);
	Button->OnHovered.AddDynamic(this, &UW_FocusableButton::HandleHovered);
	Button->OnUnhovered.AddDynamic(this, &UW_FocusableButton::HandleUnhovered);
}

void UW_FocusableButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	RefreshButtonStyle();
}

FReply UW_FocusableButton::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)//For visible widget, not for NonTestable
{
	const FKey PressedKey = InKeyEvent.GetKey();

	if (PressedKey == EKeys::Enter || PressedKey == EKeys::SpaceBar)
	{
		HandlePressed();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UW_FocusableButton::HandleClicked()
{
	OnClicked.Broadcast();
}

void UW_FocusableButton::HandlePressed()
{
	OnPressed.Broadcast();
}

void UW_FocusableButton::HandleReleased()
{
	OnReleased.Broadcast();
}

void UW_FocusableButton::HandleHovered()
{
	OnHovered.Broadcast();
}

void UW_FocusableButton::HandleUnhovered()
{
	OnUnhovered.Broadcast();
}

void UW_FocusableButton::RefreshButtonStyle()
{
	Button->SetStyle(Style);  
	Button->SetBackgroundColor(BackgroundColor);
	//Button->BackgroundColor = BackgroundColor;  //'UButton::BackgroundColor': Direct access to BackgroundColor is deprecated. Please use the getter and setter. Please update your code to the new API before upgrading to the next release, otherwise your project will no longer compile.
	
	TextBlock->SetText(Text);
	TextBlock->SetFont(Font);
	TextBlock->SetColorAndOpacity(TextColor);
	TextBlock->SetJustification(Justification);

	Button->SynchronizeProperties();
}