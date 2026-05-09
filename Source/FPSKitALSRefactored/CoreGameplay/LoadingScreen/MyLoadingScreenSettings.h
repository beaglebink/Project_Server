#pragma once

#include "CoreMinimal.h"
#include "LoadingScreenManager.h"
#include "SDefaultLoadingWidget.h"
#include "MyLoadingScreenSettings.generated.h"

UCLASS(Blueprintable, BlueprintType)
class FPSKITALSREFACTORED_API UMyLoadingScreenSettings : public ULoadingScreenSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Appearance")
	FLinearColor BackgroundColor = FLinearColor::Black;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Appearance")
	FText LoadingText = NSLOCTEXT("LoadingScreen", "Loading", "Loading...");

	virtual TSharedPtr<SWidget> CreateLoadingWidget() override
	{
		return SNew(SDefaultLoadingWidget)
			.BackgroundColor(BackgroundColor)
			.LoadingText(LoadingText);
	}
};