#include "FPSKitGameInstance.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"

// Если ALS предоставляет кастомный GVC, подключите его заголовок (раскомментируйте при необходимости)
// #include "ALSGameViewportClient.h"

void UFPSKitGameInstance::Init()
{
	Super::Init();

	// Попытка передать класс виджета в GVC (если он уже есть)
	TryUpdateGVC();

	// Подписка: вызывается после инициализации world (включая SeamlessTravel)
	FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UFPSKitGameInstance::OnPostWorldInit);
}

void UFPSKitGameInstance::Shutdown()
{
	// Убираем подписки
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);

	// Если по каким-то причинам loading screen всё ещё активен — скроем
	HideLoadingScreen();

	Super::Shutdown();
}

void UFPSKitGameInstance::TryUpdateGVC()
{
	if (!GEngine || !GEngine->GameViewport) return;

	// Если у вас есть кастомный ALSGameViewportClient, попробуйте раскомментировать include выше
	// и этот блок будет работать.
	if (/*UALSGameViewportClient*/ false) // <- замените на реальную проверку, если подключите заголовок
	{
		// Пример:
		// if (UALSGameViewportClient* MyGVC = Cast<UALSGameViewportClient>(GEngine->GameViewport))
		// {
		//     if (LoadingScreenWidgetClass && !MyGVC->LoadingScreenWidgetClass)
		//     {
		//         MyGVC->SetLoadingScreenWidgetClass(LoadingScreenWidgetClass);
		//         UE_LOG(LogTemp, Log, TEXT("FPSKitGameInstance: passed LoadingScreenWidgetClass to ALSGameViewportClient"));
		//     }
		// }
	}
}

// Попытка показать экран через MoviePlayer (если присутствует), в противном случае через GVC fallback
void UFPSKitGameInstance::TryShowOnMoviePlayerOrGVC()
{
	// Если у нас нет желания — ничего не делаем
	if (!bLoadingScreenWanted) return;

	// В этой сборке MoviePlayer отключён — используем только GVC fallback.
	if (GEngine && GEngine->GameViewport)
	{
		// Пытаемся вызвать метод ShowLoadingScreen на GVC, если он реализован в вашем GameViewportClient.
		// Без явного include кастомного GVC используем приведение к UGameViewportClient и логируем.
		UGameViewportClient* GVC = GEngine->GameViewport;
		if (GVC)
		{
			// Если ваш кастомный GVC реализует ShowLoadingScreen/HideLoadingScreen — приведите к нему и вызовите.
			// По умолчанию попробуем AddViewportWidgetContent / CreateWidget + AddToViewport из сюда, но
			// аккуратно: создание UUserWidget без OwningPlayer может не рендериться до появления контроллера.
			if (LoadingScreenWidgetInstance && LoadingScreenWidgetInstance->IsInViewport())
			{
				UE_LOG(LogTemp, Verbose, TEXT("FPSKitGameInstance: loading widget already present"));
				return;
			}

			if (LoadingScreenWidgetClass)
			{
				LoadingScreenWidgetInstance = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass);
				if (LoadingScreenWidgetInstance)
				{
					LoadingScreenWidgetInstance->AddToViewport(INT_MAX); // наверху
					LoadingScreenWidgetInstance->SetVisibility(ESlateVisibility::Visible);
					UE_LOG(LogTemp, Log, TEXT("FPSKitGameInstance: Loading widget added to viewport (fallback, stored instance)"));
					return;
				}
			}

			UE_LOG(LogTemp, Verbose, TEXT("FPSKitGameInstance: TryShowOnMoviePlayerOrGVC - no loading widget class or creation failed"));
			return;
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("FPSKitGameInstance: TryShowOnMoviePlayerOrGVC - no GameViewport available"));
}

void UFPSKitGameInstance::ShowLoadingScreen()
{
	bLoadingScreenWanted = true;

	// Сначала передадим класс виджета в GVC (на случай, если GVC будет использоваться)
	TryUpdateGVC();

	// Показываем через GVC/fallback
	TryShowOnMoviePlayerOrGVC();

	UE_LOG(LogTemp, Log, TEXT("FPSKitGameInstance: ShowLoadingScreen() requested (flag set)"));
}

void UFPSKitGameInstance::HideLoadingScreen()
{
	bLoadingScreenWanted = false;

	// Попытка удалить ранее добавленный виджет (если мы его создали)
	if (GEngine && GEngine->GameViewport)
	{
		if (LoadingScreenWidgetInstance)
		{
			if (LoadingScreenWidgetInstance->IsInViewport())
			{
				LoadingScreenWidgetInstance->RemoveFromParent();
			}
			LoadingScreenWidgetInstance = nullptr;
			UE_LOG(LogTemp, Log, TEXT("FPSKitGameInstance: removed stored loading widget"));
			return;
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("FPSKitGameInstance: HideLoadingScreen() - fallback removal attempted"));
}

// Adapter: вызывается после инициализации world — делегируем в старый обработчик
void UFPSKitGameInstance::OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
	// Вызов старого метода, простая адаптация
	OnPostLoadMapWithWorld(World);
}

// После загрузки уровня — если флаг выставлен, снова покажем экран (в новом world)
// Это гарантирует, что экран будет покрывать этапы инициализации/настроек до тех пор,
// пока вы явно не вызовете HideLoadingScreen()
void UFPSKitGameInstance::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld || !LoadedWorld->IsGameWorld()) return;

	// Обновим GVC ссылку (noop пока)
	TryUpdateGVC();

	if (bLoadingScreenWanted)
	{
		// Небольшая отложенная попытка (даёт Slate/Viewport время стать готовыми)
		constexpr float Delay = 0.05f;
		FTimerHandle TmpHandle;
		LoadedWorld->GetTimerManager().SetTimer(TmpHandle, FTimerDelegate::CreateUObject(this, &UFPSKitGameInstance::TryShowOnMoviePlayerOrGVC), Delay, false);
		UE_LOG(LogTemp, Log, TEXT("FPSKitGameInstance: OnPostLoadMapWithWorld - scheduled TryShowOnMoviePlayerOrGVC"));
	}
}