#pragma once

#include "CoreMinimal.h"
#include "LoadingScreenManager.h"
#include "SDefaultLoadingWidget.h"
#include "MyLoadingScreenSettings.generated.h"

/**
 * Пример настраиваемого класса лоадскрина.
 * Создай Blueprint на основе этого класса и настрой параметры в редакторе.
 * Затем передай экземпляр в ULoadingScreenManager::SetLoadingScreenSettings().
 *
 * Для полностью кастомного вида — переопредели CreateLoadingWidget()
 * и верни свой SWidget.
 */
UCLASS(Blueprintable, BlueprintType)
class FPSKITALSREFACTORED_API UMyLoadingScreenSettings : public ULoadingScreenSettings
{
    GENERATED_BODY()

public:
    /** Цвет фона лоадскрина */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Appearance")
    FLinearColor BackgroundColor = FLinearColor::Black;

    /** Текст под спиннером */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Appearance")
    FText LoadingText = NSLOCTEXT("LoadingScreen", "Loading", "Loading...");

    // Переопределяем создание виджета
    virtual TSharedPtr<SWidget> CreateLoadingWidget() override
    {
        return SNew(SDefaultLoadingWidget)
            .BackgroundColor(BackgroundColor)
            .LoadingText(LoadingText);
    }
};
