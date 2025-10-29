#include "BaseWidget.h"
#include "Input/Events.h"
#include "Framework/Application/SlateApplication.h"

FReply UBaseWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();
	FName KeyName = Key.GetFName();

	//UE_LOG(LogTemp, Warning, TEXT("Preview Key pressed: %s"), *KeyName.ToString());

	if (Key == EKeys::Enter && InKeyEvent.IsControlDown())
	{
		OnCombinationPressed.Broadcast(FString(TEXT("Ctrl+Enter")));

		//UE_LOG(LogTemp, Warning, TEXT("Ctrl+Enter combination detected"));
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}
