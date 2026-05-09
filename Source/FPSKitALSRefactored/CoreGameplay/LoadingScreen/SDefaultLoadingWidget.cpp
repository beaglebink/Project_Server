#include "SDefaultLoadingWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SBoxPanel.h"
#include "Styling/CoreStyle.h"
#include "Math/TransformCalculus2D.h"

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
					SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						.Padding(0.0f, 0.0f, 0.0f, 16.0f)
						[
							SNew(SBox)
								.WidthOverride(48.0f)
								.HeightOverride(48.0f)
								[
									SNew(SImage)
										.Image(FCoreStyle::Get().GetBrush("Throbber.Chunk"))
										.ColorAndOpacity(FLinearColor::White)
										.RenderTransform_Lambda([this]() { return GetSpinnerTransform(); })
										.RenderTransformPivot(FVector2D(0.5f, 0.5f))
								]
						]
					+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
								.Text(LoadingLabel)
								.ColorAndOpacity(FLinearColor::White)
								.Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
						]
				]
		];

	SetVisibility(EVisibility::HitTestInvisible);
}

void SDefaultLoadingWidget::Tick(const FGeometry& AllottedGeometry,
	const double InCurrentTime,
	const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	SpinnerAngle += InDeltaTime * 180.0f;
	if (SpinnerAngle >= 360.0f) SpinnerAngle -= 360.0f;
}

FSlateRenderTransform SDefaultLoadingWidget::GetSpinnerTransform() const
{
	const float AngleRad = FMath::DegreesToRadians(SpinnerAngle);
	return FSlateRenderTransform(FQuat2D(AngleRad));
}