#include "Inventory/UI/W_ItemSlot.h"
#include "Inventory/UI/W_VisualDescription.h"
#include "Inventory/UI/W_InventoryHUD.h"
#include "Components/SizeBox.h"

void UW_ItemSlot::NativeConstruct()
{
	Super::NativeConstruct();
}

FText UW_ItemSlot::FormatFloatFixed(float Value, int32 Precision)
{
	Precision = FMath::Clamp(Precision, 0, 4);
	FString FloatToString;
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

void UW_ItemSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::OnMouseEnter(InGeometry, InMouseEvent);

	if (UW_VisualDescription* VisualDescriptionWidget = CreateWidget<UW_VisualDescription>(GetWorld(), VisualDescriptionWidgetClass))
	{
		InventoryHUDRef->SizeBox_VisualAndDescription->AddChild(VisualDescriptionWidget);
	}
}

void UW_ItemSlot::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::OnMouseLeave(InMouseEvent);

	InventoryHUDRef->SizeBox_VisualAndDescription->ClearChildren();
}
