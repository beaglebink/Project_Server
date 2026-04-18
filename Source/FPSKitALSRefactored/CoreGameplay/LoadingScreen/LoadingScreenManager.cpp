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

    if (IGameMoviePlayer* MP = GetMoviePlayer())
    {
        OnPrepareHandle = MP->OnPrepareLoadingScreen().AddWeakLambda(this, [this]()
        {
            if (!bIsVisible) return;

            IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
            if (!MoviePlayer) return;

            TSharedPtr<SWidget> LoadingWidget = Settings ? Settings->CreateLoadingWidget() : SNullWidget::NullWidget;

            FLoadingScreenAttributes Attrs;
            // КЛЮЧЕВОЕ: AutoComplete=true — MoviePlayer НЕ блокирует game thread
            // Он сам скроется после загрузки уровня, не ожидая StopMovie()
            Attrs.bAutoCompleteWhenLoadingCompletes = true;
            Attrs.bWaitForManualStop = false;
            Attrs.WidgetLoadingScreen = LoadingWidget;
            // MinimumLoadingScreenDisplayTime у MoviePlayer не ставим — управляем временем сами через FTicker
            Attrs.MinimumLoadingScreenDisplayTime = 0.0f;
            MoviePlayer->SetupLoadingScreen(Attrs);

            UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: OnPrepareLoadingScreen — attrs set (AutoComplete=true, WaitForManualStop=false)"));
        });
    }
}

void ULoadingScreenManager::Deinitialize()
{
    if (IGameMoviePlayer* MP = GetMoviePlayer())
    {
        MP->OnPrepareLoadingScreen().Remove(OnPrepareHandle);
    }

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

    // Настраиваем атрибуты MoviePlayer заранее (OnPrepareLoadingScreen подхватит при SeamlessTravel)
    if (IGameMoviePlayer* MP = GetMoviePlayer())
    {
        FLoadingScreenAttributes Attrs;
        Attrs.bAutoCompleteWhenLoadingCompletes = true;
        Attrs.bWaitForManualStop = false;
        Attrs.WidgetLoadingScreen = LoadingWidget;
        Attrs.MinimumLoadingScreenDisplayTime = 0.0f;
        MP->SetupLoadingScreen(Attrs);
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: ShowLoadingScreen — MoviePlayer SetupLoadingScreen (AutoComplete=true)"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: ShowLoadingScreen — MoviePlayer недоступен"));
    }

    // Viewport fallback показываем ВСЕГДА — он закрывает разрыв до того как MoviePlayer включится
    // и остаётся до явного вызова HideLoadingScreen() после настройки сцены
    if (LoadingWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        RemoveViewportFallbackWidget();
        GEngine->GameViewport->AddViewportWidgetContent(LoadingWidget.ToSharedRef(), ViewportFallbackZOrder);
        ViewportFallbackWidget = LoadingWidget;
        bViewportFallback = true;
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: viewport fallback добавлен (ZOrder=%d)"), ViewportFallbackZOrder);
    }
    else if (!GEngine || !GEngine->GameViewport)
    {
        UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: GameViewport недоступен для fallback"));
    }

    bIsVisible = true;
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: ShowLoadingScreen — done"));
}

void ULoadingScreenManager::HideLoadingScreen()
{
    if (!bIsVisible)
    {
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager::HideLoadingScreen — уже скрыт, пропускаем"));
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

    // MoviePlayer с AutoComplete=true сам остановился после загрузки — StopMovie не нужен
    // Но на всякий случай останавливаем если ещё играет
    if (IGameMoviePlayer* MP = GetMoviePlayer())
    {
        if (MP->IsMovieCurrentlyPlaying())
        {
            MP->StopMovie();
            UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: StopMovie() вызван"));
        }
    }

    // Viewport fallback убираем здесь — это главный экран поверх настройки сцены
    RemoveViewportFallbackWidget();

    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин скрыт"));
}

void ULoadingScreenManager::OnMoviePlaybackFinished()
{
    // MoviePlayer завершился сам (AutoComplete) — НЕ скрываем viewport fallback
    // Fallback убирается только явным вызовом HideLoadingScreen() из Blueprint
    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: OnMoviePlaybackFinished (fallback остаётся до HideLoadingScreen)"));
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
