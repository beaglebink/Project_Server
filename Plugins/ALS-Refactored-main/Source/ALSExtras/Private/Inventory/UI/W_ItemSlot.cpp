#include "Inventory/UI/W_ItemSlot.h"
#include "Inventory/UI/W_VisualDescription.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"

void UW_ItemSlot::NativeConstruct()
{
	Super::NativeConstruct();

	bIsFocusable = true;
	SetKeyboardFocus();
}

FText UW_ItemSlot::FormatFloatFixed(float Value, int32 Precision)
{
	Precision = FMath::Clamp(Precision, 0, 4);
	FString FloatToString;

	if (Value == FMath::RoundToInt32(Value))
	{
		return FText::FromString(FString::Printf(TEXT("%.0f"), Value));
	}

	switch (Precision)
	{
	case 0:
	{
		FloatToString = FString::Printf(TEXT("%.0f"), Value);
		break;
	}
	case 1:
	{
		FloatToString = FString::Printf(TEXT("%.1f"), Value);
		break;
	}
	case 2:
	{
		FloatToString = FString::Printf(TEXT("%.2f"), Value);
		break;
	}
	case 3:
	{
		FloatToString = FString::Printf(TEXT("%.3f"), Value);
		break;
	}
	case 4:
	{
		FloatToString = FString::Printf(TEXT("%.4f"), Value);
		break;
	}
	default:
		break;
	}
	return FText::FromString(FloatToString);
}

void UW_ItemSlot::SetTintOnSelected(bool IsSet)
{
	if (IsSet)
	{
		Image_Background->SetBrushTintColor(FLinearColor(0.5f, 0.5f, 0.5f));

		if (UW_VisualDescription* VisualDescriptionWidget = CreateWidget<UW_VisualDescription>(GetWorld(), VisualDescriptionWidgetClass))
		{
			VisualDescriptionWidget->ItemName = Item.Name;
			InventoryHUDRef->SizeBox_VisualAndDescription->AddChild(VisualDescriptionWidget);
		}
	}
	else
	{
		Image_Background->SetBrushTintColor(FLinearColor(1.0f, 1.0f, 1.0f));

		InventoryHUDRef->SizeBox_VisualAndDescription->ClearChildren();
	}
}

void UW_ItemSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::OnMouseEnter(InGeometry, InMouseEvent);

	SetTintOnSelected(true);
}

void UW_ItemSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::OnMouseLeave(InMouseEvent);

	SetTintOnSelected(false);
}

FReply UW_ItemSlot::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	FReply Reply = Super::NativeOnFocusReceived(InGeometry, InFocusEvent);

	SetTintOnSelected(true);

	return Reply;
}

void UW_ItemSlot::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	SetTintOnSelected(false);
}
