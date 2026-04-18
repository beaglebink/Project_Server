#include "LoadingScreenManager.h"
#include "MoviePlayer.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Containers/Ticker.h"

void ULoadingScreenManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    OnPrepareHandle = GetMoviePlayer()->OnPrepareLoadingScreen().AddWeakLambda(
        this, [this]()
        {
            if (bIsVisible)
            {
                if (GetMoviePlayer())
                {
                    TSharedPtr<SWidget> LoadingWidget = Settings ? Settings->CreateLoadingWidget() : SNullWidget::NullWidget;

                    FLoadingScreenAttributes Attrs;
                    Attrs.bAutoCompleteWhenLoadingCompletes = Settings ? Settings->bAutoHideOnLoadComplete : false;
                    Attrs.bWaitForManualStop = !(Settings ? Settings->bAutoHideOnLoadComplete : false);
                    Attrs.WidgetLoadingScreen = LoadingWidget;
                    Attrs.MinimumLoadingScreenDisplayTime = Settings ? Settings->MinimumLoadingTime : 0.0f;
                    GetMoviePlayer()->SetupLoadingScreen(Attrs);

                    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: re-registered loading attributes on OnPrepareLoadingScreen"));
                }
            }
        });
}

void ULoadingScreenManager::Deinitialize()
{
    if (GetMoviePlayer())
        GetMoviePlayer()->OnPrepareLoadingScreen().Remove(OnPrepareHandle);

    // Отменяем отложенное скрытие если оно было
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
    IGameMoviePlayer* MP = GetMoviePlayer();
    if (!MP)
    {
        UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: MoviePlayer == nullptr. Will use viewport fallback."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: MoviePlayer IsInitialized=%d"), MP->IsInitialized());
    }

    if (!bIsVisible)
    {
        ShowTime = FPlatformTime::Seconds();
    }

    bHidePending = false;

    TSharedPtr<SWidget> LoadingWidget = Settings ? Settings->CreateLoadingWidget() : SNullWidget::NullWidget;

    FLoadingScreenAttributes Attrs;
    Attrs.bAutoCompleteWhenLoadingCompletes = Settings ? Settings->bAutoHideOnLoadComplete : false;
    Attrs.bWaitForManualStop = !(Settings ? Settings->bAutoHideOnLoadComplete : false);
    Attrs.WidgetLoadingScreen = LoadingWidget;
    Attrs.MinimumLoadingScreenDisplayTime = Settings ? Settings->MinimumLoadingTime : 0.0f;

    bool bMovieStarted = false;
    bool bPrepared = false;
    /*
    if (MP)
    {
        MP->SetupLoadingScreen(Attrs);
        bMovieStarted = MP->PlayMovie();
        bPrepared = MP->LoadingScreenIsPrepared();
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: PlayMovie=%d, IsPrepared=%d"), bMovieStarted ? 1 : 0, bPrepared ? 1 : 0);
    }

    if ((!MP || !bMovieStarted || !bPrepared) && LoadingWidget.IsValid())
    {
    */
        if (GEngine && GEngine->GameViewport)
        {
            RemoveViewportFallbackWidget();
            GEngine->GameViewport->AddViewportWidgetContent(LoadingWidget.ToSharedRef(), ViewportFallbackZOrder);
            ViewportFallbackWidget = LoadingWidget;
            bViewportFallback = true;
            UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: viewport fallback добавлен (ZOrder=%d)"), ViewportFallbackZOrder);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: GameViewport недоступен для fallback"));
        }
    //}

    bIsVisible = true;
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: ShowLoadingScreen — done (fallback=%d)"), bViewportFallback ? 1 : 0);
}

void ULoadingScreenManager::HideLoadingScreen()
{
    if (!bIsVisible)
    {
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreen — уже скрыт, пропускаем"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreen вызван"));

    // Отменяем отложенный тикер если был запущен
    if (HideTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
        HideTickerHandle.Reset();
    }

    HideLoadingScreenInternal();
}

void ULoadingScreenManager::HideLoadingScreenInternal()
{
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreenInternal — скрываем"));

    bHidePending = false;
    bIsVisible = false;

    // Отменяем тикер если ещё активен
    if (HideTickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(HideTickerHandle);
        HideTickerHandle.Reset();
    }

    IGameMoviePlayer* MP = GetMoviePlayer();
    if (MP && MP->IsMovieCurrentlyPlaying())
    {
        MP->StopMovie();
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: StopMovie() вызван"));
    }

    RemoveViewportFallbackWidget();
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин скрыт"));
}

void ULoadingScreenManager::OnMoviePlaybackFinished()
{
    if (bIsVisible && !bHidePending)
        bIsVisible = false;
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
