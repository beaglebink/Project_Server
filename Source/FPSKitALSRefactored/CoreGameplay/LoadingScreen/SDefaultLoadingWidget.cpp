#include "SDefaultLoadingWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"

void SDefaultLoadingWidget::Construct(const FArguments& InArgs)
{
	const FLinearColor BgColor = InArgs._BackgroundColor.IsSet()
		? InArgs._BackgroundColor.Get()
		: FLinearColor::Black;

	const FText LoadingLabel = InArgs._LoadingText.IsSet()
		? InArgs._LoadingText.Get()
		: NSLOCTEXT("LoadingScreen", "Loading", "Loading...");

	ChildSlot
		[
			SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SColorBlock)
						.Color(BgColor)
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.Text(LoadingLabel)
						.ColorAndOpacity(FLinearColor::White)
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 24))
				]
		];

	SetVisibility(EVisibility::HitTestInvisible);
}