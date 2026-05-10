#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FPSKITALSREFACTORED_API SDefaultLoadingWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDefaultLoadingWidget) {}
		SLATE_ATTRIBUTE(FLinearColor, BackgroundColor)
		SLATE_ATTRIBUTE(FText, LoadingText)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
};