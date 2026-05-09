#include "LoadingScreenManager.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Slate/SceneViewport.h"

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

    // Снимаем захват мыши
    if (GEngine && GEngine->GameViewport)
    {
        if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->GetGameViewport()))
        {
            SceneViewport->CaptureMouse(false);
        }
    }

    // Восстанавливаем видимость курсора
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            PC->bShowMouseCursor = bSavedMouseCursorVisible;
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

    if (LoadingWindow.IsValid())
    {
        DestroyLoadingWindow();
    }

    CreateLoadingWindow(LoadingWidget);

    bViewportFallback = true;
    bIsVisible = true;
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин показан (IsVisible=%d)"), bIsVisible ? 1 : 0);

    // Запускаем тикер-сторож фокуса
    if (!FocusGuardHandle.IsValid())
    {
        FocusGuardHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &ULoadingScreenManager::FocusGuardTick), 0.1f);
    }

    // --- Блокировка мыши ---
    if (GEngine && GEngine->GameViewport)
    {
        if (FSceneViewport* SceneViewport = static_cast<FSceneViewport*>(GEngine->GameViewport->GetGameViewport()))
        {
            SceneViewport->CaptureMouse(true);
        }
    }

    // Сохраняем видимость курсора и скрываем его
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            bSavedMouseCursorVisible = PC->bShowMouseCursor;
            PC->bShowMouseCursor = false;
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
    if (FocusGuardHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(FocusGuardHandle);
        FocusGuardHandle.Reset();
    }

    DestroyLoadingWindow();
    RemoveViewportFallbackWidget();

    // --- Восстановление мыши ---
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

void ULoadingScreenManager::OnMoviePlaybackFinished() {}

void ULoadingScreenManager::CreateLoadingWindow(const TSharedPtr<SWidget>& Content)
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
        .SupportsTransparency(EWindowTransparency::PerWindow)   // разрешаем прозрачность фона
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

    // Возвращаем фокус игровому окну
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

        // Возвращаем фокус игре
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