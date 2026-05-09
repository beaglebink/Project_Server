#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

class FPSKITALSREFACTORED_API SDefaultLoadingWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDefaultLoadingWidget) {}
		SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
		SLATE_ATTRIBUTE(FText, LoadingText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry,
		const double InCurrentTime,
		const float InDeltaTime) override;

private:
	float SpinnerAngle = 0.0f;
	FSlateRenderTransform GetSpinnerTransform() const;
};