#include "LoadingScreenManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"

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

	if (FocusGuardHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FocusGuardHandle);
		FocusGuardHandle.Reset();
	}

	DestroyLoadingWindow();
	RemoveViewportFallbackWidget();
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

	if (LoadingWindow.IsValid())
	{
		DestroyLoadingWindow();
	}

	CreateLoadingWindow(LoadingWidget);

	bViewportFallback = true;
	bIsVisible = true;
	UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин показан (IsVisible=%d)"), bIsVisible ? 1 : 0);

	// Запускаем тикер-сторож для постоянного возврата фокуса и скрытия курсора
	if (!FocusGuardHandle.IsValid())
	{
		FocusGuardHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &ULoadingScreenManager::FocusGuardTick), 0.1f);
	}
}

void ULoadingScreenManager::HideLoadingScreen()
{
	if (!bIsVisible)
	{
		UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreen — уже скрыт"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreen вызван"));

	if (HideTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
		HideTickerHandle.Reset();
	}

	HideLoadingScreenInternal();
}

void ULoadingScreenManager::HideLoadingScreenInternal()
{
	UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreenInternal"));

	bHidePending = false;
	bIsVisible = false;

	if (HideTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
		HideTickerHandle.Reset();
	}

	// Останавливаем сторож-тикер
	if (FocusGuardHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(FocusGuardHandle);
		FocusGuardHandle.Reset();
	}

	DestroyLoadingWindow();
	RemoveViewportFallbackWidget();
	UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин скрыт"));
}

void ULoadingScreenManager::OnMoviePlaybackFinished()
{
}

void ULoadingScreenManager::CreateLoadingWindow(const TSharedPtr<SWidget>& Content)
{
	// Получаем реальное положение и размер игрового окна
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

	// Создаём безрамочное окно поверх игры
	LoadingWindow = SNew(SWindow)
		.Type(EWindowType::GameWindow)
		.Style(&FCoreStyle::Get().GetWidgetStyle<FWindowStyle>("Window"))
		.Title(FText::GetEmpty())
		.ScreenPosition(WindowPos)
		.ClientSize(WindowSize)
		.SupportsTransparency(EWindowTransparency::None)
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

	// Возвращаем фокус игре без скрытия курсора
	if (GEngine && GEngine->GameViewport)
	{
		TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow();
		if (GameWindow.IsValid())
		{
			GameWindow->BringToFront(true);
		}
	}
}

void ULoadingScreenManager::DestroyLoadingWindow()
{
	if (LoadingWindow.IsValid())
	{
		LoadingWindow->RequestDestroyWindow();
		LoadingWindow.Reset();

		// После закрытия фокус возвращается игре, курсор не трогаем
		if (GEngine && GEngine->GameViewport)
		{
			TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow();
			if (GameWindow.IsValid())
			{
				GameWindow->BringToFront(true);
			}
		}
	}
}

bool ULoadingScreenManager::FocusGuardTick(float DeltaTime)
{
	if (!bIsVisible) return false;

	// Всегда возвращаем фокус игре, пока висит лоадскрин
	if (GEngine && GEngine->GameViewport)
	{
		TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow();
		if (GameWindow.IsValid())
		{
			GameWindow->BringToFront(true);
		}
	}
	return true;
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