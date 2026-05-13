#include "FPSKitGameInstance.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"
#include "LoadingScreen/LoadingScreenManager.h" // <-- ÄÎÁÀÂÈÒÜ

void UFPSKitGameInstance::Init()
{
	Super::Init();

	ULoadingScreenManager* Manager = GetSubsystem<ULoadingScreenManager>();
	if (!Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFPSKitGameInstance::Init — LoadingScreenManager subsystem NOT found"));
		return;
	}

	ULoadingScreenSettings* SettingsObj = nullptr;
	if (LoadingScreenSettingsClass)
	{
		SettingsObj = NewObject<ULoadingScreenSettings>(this, LoadingScreenSettingsClass);
	}
	else
	{
		UMyLoadingScreenSettings* DefaultSettings = NewObject<UMyLoadingScreenSettings>(this);
		DefaultSettings->BackgroundColor = FLinearColor::Black;
		DefaultSettings->LoadingText = NSLOCTEXT("LoadingScreen", "Loading", "Çàãðóçêà...");
		DefaultSettings->MinimumLoadingTime = 1.0f;
		DefaultSettings->bAutoHideOnLoadComplete = false;
		SettingsObj = DefaultSettings;
	}

	Manager->SetLoadingScreenSettings(SettingsObj);
	UE_LOG(LogTemp, Log, TEXT("UFPSKitGameInstance::Init — LoadingScreenManager configured"));
}

void UFPSKitGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UFPSKitGameInstance::ShowLoadscreen()
{
	ULoadingScreenManager* Manager = GetSubsystem<ULoadingScreenManager>();
	if (Manager)
	{
		Manager->ShowLoadingScreen();
		UE_LOG(LogTemp, Log, TEXT("UFPSKitGameInstance::TravelToLevel — ShowLoadingScreen called"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UFPSKitGameInstance::TravelToLevel — LoadingScreenManager NOT found"));
	}
}

void UFPSKitGameInstance::HideLoadscreen()
{
	UE_LOG(LogTemp, Log, TEXT("UFPSKitGameInstance::OnLevelReady — called"));

	ULoadingScreenManager* Manager = GetSubsystem<ULoadingScreenManager>();
	if (!Manager)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFPSKitGameInstance::OnLevelReady — LoadingScreenManager NOT found"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("UFPSKitGameInstance::OnLevelReady — IsVisible=%d, calling HideLoadingScreen"),
		Manager->IsLoadingScreenVisible() ? 1 : 0);

	Manager->HideLoadingScreen();
}
