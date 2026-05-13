#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include <LoadingScreen/MyLoadingScreenSettings.h>
#include "FPSKitGameInstance.generated.h"

UCLASS()
class UFPSKitGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading")
	TSubclassOf<UUserWidget> LoadingScreenWidgetClass;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Loading")
	bool bLoadingScreenWanted = false;

	UPROPERTY(EditDefaultsOnly, Category = "Loading Screen")
	TSubclassOf<ULoadingScreenSettings> LoadingScreenSettingsClass;

	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void ShowLoadscreen();

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void HideLoadscreen();

private:
	UPROPERTY(Transient)
	UUserWidget* LoadingScreenWidgetInstance = nullptr;
};