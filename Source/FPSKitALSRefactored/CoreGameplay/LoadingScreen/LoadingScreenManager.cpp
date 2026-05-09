#include "LoadingScreenManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SWindow.h"
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

    DestroyLoadingWindow();          // <-- Уничтожаем окно
    RemoveViewportFallbackWidget();  // Очистка старого fallback
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

    // Удаляем предыдущее окно, если осталось
    if (LoadingWindow.IsValid())
    {
        DestroyLoadingWindow();
    }

    CreateLoadingWindow(LoadingWidget);

    bViewportFallback = true;      // для обратной совместимости с логами
    bIsVisible = true;
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин показан (IsVisible=%d)"), bIsVisible ? 1 : 0);
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

    DestroyLoadingWindow();
    RemoveViewportFallbackWidget();
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин скрыт"));
}

void ULoadingScreenManager::OnMoviePlaybackFinished()
{
}

void ULoadingScreenManager::CreateLoadingWindow(const TSharedPtr<SWidget>& Content)
{
    FVector2D WindowSize(1920, 1080);
    FVector2D WindowPos = FVector2D::ZeroVector;

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->GetViewportSize(WindowSize);   // берём размер вьюпорта как запасной

        TSharedPtr<SWindow> GameWindow = GEngine->GameViewport->GetWindow();
        if (GameWindow.IsValid())
        {
            // Координаты игрового окна на экране
            WindowPos = GameWindow->GetPositionInScreen();
            // Реальный клиентский размер окна (может отличаться из-за масштабирования)
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
        .ScreenPosition(WindowPos)                // ← позиционируем в точку игрового окна
        .ClientSize(WindowSize)
        .SupportsTransparency(EWindowTransparency::None)
        .IsInitiallyMaximized(false)
        .IsTopmostWindow(true)
        .IsPopupWindow(false)
        .CreateTitleBar(false)
        .SaneWindowPlacement(false)
        .FocusWhenFirstShown(false)
        .UseOSWindowBorder(false);

    LoadingWindow->SetContent(Content.ToSharedRef());

    LoadingWindow->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this](const TSharedRef<SWindow>&)
        {
            LoadingWindow.Reset();
        }));

    FSlateApplication::Get().AddWindow(LoadingWindow.ToSharedRef(), true);
}

void ULoadingScreenManager::DestroyLoadingWindow()
{
    if (LoadingWindow.IsValid())
    {
        LoadingWindow->RequestDestroyWindow();
        LoadingWindow.Reset();
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