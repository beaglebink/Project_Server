#include "LoadingScreenManager.h"
#include "MoviePlayer.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"

void ULoadingScreenManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // MoviePlayer не используем для SeamlessTravel — он не совместим с ним
}

void ULoadingScreenManager::Deinitialize()
{
    if (HideTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
        HideTickerHandle.Reset();
    }
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

    if (LoadingWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        RemoveViewportFallbackWidget();
        GEngine->GameViewport->AddViewportWidgetContent(LoadingWidget.ToSharedRef(), ViewportFallbackZOrder);
        ViewportFallbackWidget = LoadingWidget;
        bViewportFallback = true;
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: viewport fallback добавлен (ZOrder=%d)"), ViewportFallbackZOrder);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: GameViewport недоступен, fallback не добавлен"));
    }

    bIsVisible = true;
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: ShowLoadingScreen done (IsVisible=%d, Fallback=%d)"),
        bIsVisible ? 1 : 0, bViewportFallback ? 1 : 0);
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

    RemoveViewportFallbackWidget();
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин скрыт"));
}

void ULoadingScreenManager::OnMoviePlaybackFinished()
{
}

void ULoadingScreenManager::RemoveViewportFallbackWidget()
{
    if (bViewportFallback && ViewportFallbackWidget.IsValid())
    {
        if (GEngine && GEngine->GameViewport)
        {
            GEngine->GameViewport->RemoveViewportWidgetContent(ViewportFallbackWidget.ToSharedRef());
            UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: viewport fallback удалён"));
        }
        ViewportFallbackWidget.Reset();
    }
    bViewportFallback = false;
}
