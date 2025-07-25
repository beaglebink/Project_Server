#include "Inventory/UI/W_NotEnough.h"
#include "Animation/WidgetAnimation.h"

void UW_NotEnough::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UW_NotEnough::PlayAppearing()
{
	PlayAnimation(Appearing);
}
