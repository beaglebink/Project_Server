#include "FPSKitGameInstance.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"

void UFPSKitGameInstance::Init()
{
	Super::Init();

    // Получаем менеджер
    ULoadingScreenManager* Manager = GetSubsystem<ULoadingScreenManager>();
    if (!Manager) return;

    // Создаём настройки (можно задать класс через Blueprint в редакторе)
    ULoadingScreenSettings* SettingsObj = nullptr;

    if (LoadingScreenSettingsClass)
    {
        // Класс задан через редактор — создаём экземпляр
        SettingsObj = NewObject<ULoadingScreenSettings>(this, LoadingScreenSettingsClass);
    }
    else
    {
        // Фоллбэк — дефолтный чёрный экран
        UMyLoadingScreenSettings* DefaultSettings =
            NewObject<UMyLoadingScreenSettings>(this);
        DefaultSettings->BackgroundColor = FLinearColor::Black;
        DefaultSettings->LoadingText =
            NSLOCTEXT("LoadingScreen", "Loading", "Загрузка...");
        DefaultSettings->MinimumLoadingTime = 1.0f; // минимум 1 секунда
        DefaultSettings->bAutoHideOnLoadComplete = false;
        SettingsObj = DefaultSettings;
    }

    Manager->SetLoadingScreenSettings(SettingsObj);
}

void UFPSKitGameInstance::Shutdown()
{
	Super::Shutdown();
}

// ============================================================
// КАК ИСПОЛЬЗОВАТЬ при переходе между уровнями
// ============================================================

// 1. Показать лоадскрин ДО вызова ServerTravel/ClientTravel
void UFPSKitGameInstance::TravelToLevel(const FString& LevelName)
{
    ULoadingScreenManager* Manager = GetSubsystem<ULoadingScreenManager>();
    if (Manager)
    {
        Manager->ShowLoadingScreen();
    }

    // Seamless travel — контроллер сохраняется
    if (UWorld* World = GetWorld())
    {
        // Для сервера:
        //World->ServerTravel(LevelName + "?listen", true /*bAbsolute*/);

        // Для клиента (если нужно):
        // APlayerController* PC = World->GetFirstPlayerController();
        // if (PC) PC->ClientTravel(LevelName, TRAVEL_Absolute);
    }
}

// 2. Скрыть лоадскрин когда новый уровень готов
// Вызывай это из BeginPlay нового уровня или из GameMode::PostLogin
void UFPSKitGameInstance::OnLevelReady()
{
    ULoadingScreenManager* Manager = GetSubsystem<ULoadingScreenManager>();
    if (Manager)
    {
        Manager->HideLoadingScreen();
    }
}
