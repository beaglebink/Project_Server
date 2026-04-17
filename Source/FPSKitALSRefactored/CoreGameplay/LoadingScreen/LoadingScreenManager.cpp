#include "LoadingScreenManager.h"
#include "MoviePlayer.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Engine/Engine.h" // GEngine
#include "Engine/GameViewportClient.h"

void ULoadingScreenManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Подписываемся на событие подготовки к загрузке карты.
    // Это позволяет MoviePlayer подготовиться до начала загрузки,
    // что исключает чёрный кадр при SeamlessTravel.
    OnPrepareHandle = GetMoviePlayer()->OnPrepareLoadingScreen().AddWeakLambda(
        this, [this]()
        {
            // Если лоадскрин уже активен — переподключаем его к новому
            // MoviePlayer-у который создаётся при каждой загрузке уровня.
            // Именно здесь происходит "мост" между уровнями.
            if (bIsVisible)
            {
                // Не вызываем ShowLoadingScreen полностью — просто ре-регистрируем виджет
                // (в текущей реализации ShowLoadingScreen переустанавливает время показа,
                //  поэтому здесь сохраняем ShowTime и только регистрируем атрибуты)
                if (GetMoviePlayer())
                {
                    TSharedPtr<SWidget> LoadingWidget = SNullWidget::NullWidget;
                    if (Settings)
                    {
                        LoadingWidget = Settings->CreateLoadingWidget();
                    }

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
    {
        GetMoviePlayer()->OnPrepareLoadingScreen().Remove(OnPrepareHandle);
    }

    // Удаляем возможный viewport-fallback
    RemoveViewportFallbackWidget();

    Super::Deinitialize();
}

void ULoadingScreenManager::SetLoadingScreenSettings(ULoadingScreenSettings* InSettings)
{
    Settings = InSettings;
}

void ULoadingScreenManager::ShowLoadingScreen()
{
    // Diagnostics: MoviePlayer pointer and state
    IGameMoviePlayer* MP = GetMoviePlayer();
    if (!MP)
    {
        UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: MoviePlayer == nullptr. Will use viewport fallback if widget present."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: MoviePlayer ptr=%p. IsInitialized=%d, HasEarlyStartupMovie=%d"),
            MP, MP->IsInitialized(), MP->HasEarlyStartupMovie());
    }

    // Запоминаем время показа для MinimumLoadingTime (только при первом показе)
    if (!bIsVisible)
    {
        ShowTime = FPlatformTime::Seconds();
    }
    bHidePending = false;

    // Создаём виджет из настроек или используем пустой
    TSharedPtr<SWidget> LoadingWidget = SNullWidget::NullWidget;
    if (Settings)
    {
        LoadingWidget = Settings->CreateLoadingWidget();
    }

    // Настраиваем MoviePlayer
    FLoadingScreenAttributes Attrs;
    Attrs.bAutoCompleteWhenLoadingCompletes = Settings ? Settings->bAutoHideOnLoadComplete : false;
    Attrs.bWaitForManualStop = !(Settings ? Settings->bAutoHideOnLoadComplete : false);
    Attrs.WidgetLoadingScreen = LoadingWidget;
    Attrs.MinimumLoadingScreenDisplayTime = Settings ? Settings->MinimumLoadingTime : 0.0f;

    bool bMovieStarted = false;
    bool bPrepared = false;

    if (MP)
    {
        // Setup + Play
        MP->SetupLoadingScreen(Attrs);
        bMovieStarted = MP->PlayMovie();
        bPrepared = MP->LoadingScreenIsPrepared();
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: SetupLoadingScreen called. PlayMovie returned=%d, LoadingScreenIsPrepared=%d"),
            bMovieStarted ? 1 : 0, bPrepared ? 1 : 0);
    }

    // Если MoviePlayer не запустился или не готов — делаем фоллбек: показываем тот же Slate-виджет в GameViewport
    if ((!MP || !bMovieStarted || !bPrepared) && LoadingWidget.IsValid())
    {
        if (GEngine && GEngine->GameViewport)
        {
            // Сначала удалим предыдущий фоллбек, если есть
            RemoveViewportFallbackWidget();

            // AddViewportWidgetContent принимает TSharedRef<SWidget> и опциональный ZOrder
            TSharedRef<SWidget> WidgetRef = LoadingWidget.ToSharedRef();
            GEngine->GameViewport->AddViewportWidgetContent(WidgetRef, ViewportFallbackZOrder);
            ViewportFallbackWidget = LoadingWidget;
            bViewportFallback = true;

            UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: using viewport fallback for loading widget (MoviePlayer unavailable or did not start) ZOrder=%d"), ViewportFallbackZOrder);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ULoadingScreenManager: cannot use viewport fallback - GameViewport missing"));
        }
    }

    // Если MoviePlayer стартовал — пометим видимым; иначе всё равно считаем видимым, т.к. виджет показан через фоллбек
    bIsVisible = true;

    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин показан (bIsVisible=%d, bViewportFallback=%d)"),
        bIsVisible ? 1 : 0, bViewportFallback ? 1 : 0);
}

void ULoadingScreenManager::HideLoadingScreen()
{
    if (!bIsVisible)
    {
        return;
    }

    // Проверяем минимальное время показа
    if (Settings && Settings->MinimumLoadingTime > 0.0f)
    {
        const double Elapsed = FPlatformTime::Seconds() - ShowTime;
        const float Remaining = Settings->MinimumLoadingTime - (float)Elapsed;

        if (Remaining > 0.0f)
        {
            // Откладываем скрытие на оставшееся время
            bHidePending = true;

            if (UWorld* World = GetGameInstance()->GetWorld())
            {
                FTimerHandle TimerHandle;
                World->GetTimerManager().SetTimer(
                    TimerHandle,
                    this,
                    &ULoadingScreenManager::HideLoadingScreenInternal,
                    Remaining,
                    false
                );
            }
            return;
        }
    }

    HideLoadingScreenInternal();
}

void ULoadingScreenManager::HideLoadingScreenInternal()
{
    bHidePending = false;
    bIsVisible = false;

    // Если используется MoviePlayer — остановим его
    IGameMoviePlayer* MP = GetMoviePlayer();
    if (MP && MP->IsMovieCurrentlyPlaying())
    {
        MP->StopMovie();
        UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: requested StopMovie() on MoviePlayer"));
    }

    // Если мы использовали viewport-fallback — удалим виджет
    RemoveViewportFallbackWidget();

    UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: лоадскрин скрыт"));
}

void ULoadingScreenManager::OnMoviePlaybackFinished()
{
    if (bIsVisible && !bHidePending)
    {
        bIsVisible = false;
    }
}

void ULoadingScreenManager::RemoveViewportFallbackWidget()
{
    if (bViewportFallback && ViewportFallbackWidget.IsValid())
    {
        if (GEngine && GEngine->GameViewport)
        {
            // RemoveViewportWidgetContent требует SharedRef
            GEngine->GameViewport->RemoveViewportWidgetContent(ViewportFallbackWidget.ToSharedRef());
            UE_LOG(LogTemp, Log, TEXT("ULoadingScreenManager: removed viewport fallback widget (ZOrder=%d)"), ViewportFallbackZOrder);
        }
        ViewportFallbackWidget.Reset();
    }
    bViewportFallback = false;
}
