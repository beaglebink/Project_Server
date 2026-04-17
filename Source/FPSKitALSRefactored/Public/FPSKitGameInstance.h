#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
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

	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void ShowLoadingScreen();

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void HideLoadingScreen();

private:
	// Попытка передать класс виджета в GVC (если он уже создан)
	void TryUpdateGVC();

	// Попытка показать экран на MoviePlayer или GVC
	void TryShowOnMoviePlayerOrGVC();

	// Старый обработчик — вызывается адаптером OnPostWorldInit
	void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

	// Новый обработчик делегата FWorldDelegates::OnPostWorldInitialization
	void OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);

	UPROPERTY(Transient)
	UUserWidget* LoadingScreenWidgetInstance = nullptr;
};