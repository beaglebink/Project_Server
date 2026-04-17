#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MoviePlayer.h"
#include "LoadingScreenManager.generated.h"

/**
 * Базовый класс настроек лоадскрина.
 * Наследуйся от него чтобы задать свой виджет и параметры.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class FPSKITALSREFACTORED_API ULoadingScreenSettings : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Класс Slate-виджета для отображения.
     * Если не задан — используется дефолтный виджет (чёрный экран).
     * Переопредели CreateLoadingWidget() чтобы вернуть свой SWidget.
     */
    virtual TSharedPtr<SWidget> CreateLoadingWidget()
    {
        return SNullWidget::NullWidget;
    }

    /** Минимальное время показа лоадскрина в секундах (0 = без ограничений) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
    float MinimumLoadingTime = 0.0f;

    /** Скрывать автоматически после загрузки уровня */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Loading Screen")
    bool bAutoHideOnLoadComplete = false;
};


/**
 * Менеджер лоадскрина — GameInstance Subsystem.
 * Живёт всю сессию, управляет показом/скрытием через MoviePlayer.
 *
 * Использование:
 *   ULoadingScreenManager* Manager = GetGameInstance()->GetSubsystem<ULoadingScreenManager>();
 *   Manager->SetLoadingScreenSettings(MySettings);
 *   Manager->ShowLoadingScreen();
 *   // ... ServerTravel / ClientTravel ...
 *   Manager->HideLoadingScreen();
 */
UCLASS()
class FPSKITALSREFACTORED_API ULoadingScreenManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ~UGameInstanceSubsystem interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Задать настройки и класс виджета лоадскрина.
     * Вызывать ДО ShowLoadingScreen().
     */
    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void SetLoadingScreenSettings(ULoadingScreenSettings* InSettings);

    /**
     * Показать лоадскрин.
     * Безопасно вызывать многократно — повторный вызов игнорируется.
     */
    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void ShowLoadingScreen();

    /**
     * Скрыть лоадскрин.
     * Если задан MinimumLoadingTime — скрытие откладывается до его истечения.
     */
    UFUNCTION(BlueprintCallable, Category = "Loading Screen")
    void HideLoadingScreen();

    /** Возвращает true если лоадскрин сейчас активен */
    UFUNCTION(BlueprintPure, Category = "Loading Screen")
    bool IsLoadingScreenVisible() const { return bIsVisible; }

private:
    /** Подписка на завершение загрузки уровня */
    void OnMoviePlaybackFinished();

    /** Внутреннее скрытие после истечения минимального времени */
    void HideLoadingScreenInternal();

    /** Удаляет виджет, добавленный в GameViewport (fallback) */
    void RemoveViewportFallbackWidget();

    UPROPERTY()
    TObjectPtr<ULoadingScreenSettings> Settings;

    bool bIsVisible = false;
    double ShowTime = 0.0;
    bool bHidePending = false;

    FDelegateHandle OnPrepareHandle;

    // --- fallback when MoviePlayer is unavailable or doesn't actually present UI ---
    bool bViewportFallback = false;
    TSharedPtr<SWidget> ViewportFallbackWidget;

    // Z-order для виджета, добавляемого в GameViewport (по умолчанию высокий, чтобы быть наверху)
    int32 ViewportFallbackZOrder = 1000000;
};
