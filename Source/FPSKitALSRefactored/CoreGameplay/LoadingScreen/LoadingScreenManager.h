#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MoviePlayer.h"
#include "Containers/Ticker.h"
#include "Widgets/SWindow.h"
#include "LoadingScreenManager.generated.h"

UCLASS(Abstract, Blueprintable, BlueprintType)
class FPSKITALSREFACTORED_API ULoadingScreenSettings : public UObject
{
	GENERATED_BODY()

public:
	virtual TSharedPtr<SWidget> CreateLoadingWidget()
	{
		return SNullWidget::NullWidget;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
	float MinimumLoadingTime = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
	bool bAutoHideOnLoadComplete = false;
};

UCLASS()
class FPSKITALSREFACTORED_API ULoadingScreenManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Loading Screen")
	void SetLoadingScreenSettings(ULoadingScreenSettings* InSettings);

	UFUNCTION(BlueprintCallable, Category = "Loading Screen")
	void ShowLoadingScreen();

	UFUNCTION(BlueprintCallable, Category = "Loading Screen")
	void HideLoadingScreen();

	UFUNCTION(BlueprintPure, Category = "Loading Screen")
	bool IsLoadingScreenVisible() const { return bIsVisible; }

private:
	void OnMoviePlaybackFinished();
	void HideLoadingScreenInternal();
	void RemoveViewportFallbackWidget();

	void CreateLoadingWindow(const TSharedPtr<SWidget>& Content);
	void DestroyLoadingWindow();
	bool FocusGuardTick(float DeltaTime);

	UPROPERTY()
	TObjectPtr<ULoadingScreenSettings> Settings;

	bool bIsVisible = false;
	double ShowTime = 0.0;
	bool bHidePending = false;

	FDelegateHandle OnPrepareHandle;
	FTSTicker::FDelegateHandle HideTickerHandle;
	FTSTicker::FDelegateHandle FocusGuardHandle;

	bool bViewportFallback = false;
	TSharedPtr<SWidget> ViewportFallbackWidget;
	int32 ViewportFallbackZOrder = 1000000;

	TSharedPtr<SWindow> LoadingWindow;
};