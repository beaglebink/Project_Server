#include "LoadingScreenManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Slate/SceneViewport.h"
#include "HAL/IConsoleManager.h"

bool ULoadingScreenManager::bUseLegacyLoadingScreen = false;
static FAutoConsoleVariableRef CVarLegacyLoadingScreen(
	TEXT("LoadingScreen.UseLegacy"),
	ULoadingScreenManager::bUseLegacyLoadingScreen,
	TEXT("If true, uses legacy viewport fallback without mouse capture (like in editor). Works in shipping too."),
	ECVF_Default
);

void ULoadingScreenManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void ULoadingScreenManager::Deinitialize()
{
	if (HideTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
		HideTickerHandle.Reset();
	}

	DestroyLoadingWindow();
	RemoveViewportFallbackWidget();

	// Снимаем захват мыши и восстанавливаем курсор, только если используется новый метод
	if (!GIsEditor && !bUseLegacyLoadingScreen)
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->GetGameViewport()))
			{
				SceneViewport->CaptureMouse(false);
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				PC->bShowMouseCursor = bSavedMouseCursorVisible;
			}
		}
	}

	Super::Deinitialize();
}

void ULoadingScreenManager::SetLoadingScreenSettings(ULoadingScreenSettings* InSettings)
{
	Settings = InSettings;
}

void ULoadingScreenManager::ShowLoadingScreen()
{
	if (!bIsVisible)
	{
		ShowTime = FPlatformTime::Seconds();
	}
	bHidePending = false;

	TSharedPtr<SWidget> LoadingWidget = Settings ? Settings->CreateLoadingWidget() : SNullWidget::NullWidget;
	if (!LoadingWidget.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: не удалось создать виджет загрузки"));
		return;
	}

	if (LoadingWindow.IsValid() || (GIsEditor && bViewportFallback) || (bUseLegacyLoadingScreen && bViewportFallback))
	{
		DestroyLoadingWindow();
	}

	CreateLoadingWindow(LoadingWidget);

	bIsVisible = true;

	// Захват мыши и скрытие курсора только в новом режиме (не редактор и не legacy)
	if (!GIsEditor && !bUseLegacyLoadingScreen)
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->GetGameViewport()))
			{
				SceneViewport->CaptureMouse(true);
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				bSavedMouseCursorVisible = PC->bShowMouseCursor;
				PC->bShowMouseCursor = false;
			}
		}
	}
}

void ULoadingScreenManager::HideLoadingScreen()
{
	if (!bIsVisible) return;

	if (HideTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
		HideTickerHandle.Reset();
	}
	HideLoadingScreenInternal();
}

void ULoadingScreenManager::HideLoadingScreenInternal()
{
	bHidePending = false;
	bIsVisible = false;

	if (HideTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
		HideTickerHandle.Reset();
	}

	// Снимаем захват мыши, только если он был установлен
	if (!GIsEditor && !bUseLegacyLoadingScreen)
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->GetGameViewport()))
			{
				SceneViewport->CaptureMouse(false);
			}
		}

		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				PC->bShowMouseCursor = bSavedMouseCursorVisible;
			}
		}
	}

	DestroyLoadingWindow();
	RemoveViewportFallbackWidget();
}

void ULoadingScreenManager::OnMoviePlaybackFinished() {}

void ULoadingScreenManager::CreateLoadingWindow(const TSharedPtr<SWidget>& Content)
{
	// Используем старый viewport fallback, если редактор или включён legacy-режим
	if (GIsEditor || bUseLegacyLoadingScreen)
	{
		if (GEngine && GEngine->GameViewport)
		{
			RemoveViewportFallbackWidget();
			GEngine->GameViewport->AddViewportWidgetContent(Content.ToSharedRef(), ViewportFallbackZOrder);
			ViewportFallbackWidget = Content;
			bViewportFallback = true;
		}
	}
	else
	{
		FVector2D WindowSize(1920, 1080);
		FVector2D WindowPos = FVector2D::ZeroVector;

		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->GetViewportSize(WindowSize);

			TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow();
			if (GameWindow.IsValid())
			{
				WindowPos = GameWindow->GetPositionInScreen();
				FVector2D GameWindowSize = GameWindow->GetClientSizeInScreen();
				if (GameWindowSize.X > 0 && GameWindowSize.Y > 0)
				{
					WindowSize = GameWindowSize;
				}
			}
		}

		LoadingWindow = SNew(SWindow)
			.Type(EWindowType::GameWindow)
			.Style(&FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window"))
			.Title(FText::GetEmpty())
			.ScreenPosition(WindowPos)
			.ClientSize(WindowSize)
			.SupportsTransparency(EWindowTransparency::PerWindow)
			.IsInitiallyMaximized(false)
			.IsTopmostWindow(true)
			.IsPopupWindow(false)
			.CreateTitleBar(false)
			.SaneWindowPlacement(false)
			.FocusWhenFirstShown(false)
			.UseOSWindowBorder(false)
			.ActivationPolicy(EWindowActivationPolicy::Never);

		LoadingWindow->SetContent(Content.ToSharedRef());

		LoadingWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&)
			{
				LoadingWindow.Reset();
			}));

		FSlateApplication::Get().AddWindow(LoadingWindow.ToSharedRef(), true);
	}
}

void ULoadingScreenManager::DestroyLoadingWindow()
{
	if (GIsEditor || bUseLegacyLoadingScreen)
	{
		RemoveViewportFallbackWidget();
	}
	else
	{
		if (LoadingWindow.IsValid())
		{
			// Снимаем захват мыши перед уничтожением окна (если он был)
			if (!GIsEditor && !bUseLegacyLoadingScreen)
			{
				if (GEngine && GEngine->GameViewport)
				{
					if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->GetGameViewport()))
					{
						SceneViewport->CaptureMouse(false);
					}
				}
			}

			TSharedPtr<SWindow> WindowToDestroy = LoadingWindow;
			LoadingWindow.Reset();

			if (WindowToDestroy.IsValid())
			{
				WindowToDestroy->RequestDestroyWindow();
			}
		}
	}
}

void ULoadingScreenManager::RemoveViewportFallbackWidget()
{
	if (bViewportFallback && ViewportFallbackWidget.IsValid())
	{
		if (GEngine && GEngine->GameViewport)
		{
			GEngine->GameViewport->RemoveViewportWidgetContent(ViewportFallbackWidget.ToSharedRef());
		}
		ViewportFallbackWidget.Reset();
	}
	bViewportFallback = false;
}