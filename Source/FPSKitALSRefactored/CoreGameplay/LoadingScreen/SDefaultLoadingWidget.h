#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"

/**
 * Дефолтный Slate-виджет лоадскрина.
 * Чёрный фон + анимированный спиннер.
 *
 * Ты можешь создать свой виджет унаследовавшись от SCompoundWidget
 * и вернуть его из ULoadingScreenSettings::CreateLoadingWidget().
 */
class FPSKITALSREFACTORED_API SDefaultLoadingWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDefaultLoadingWidget) {}
        /** Цвет фона */
        SLATE_ATTRIBUTE(FLinearColor, BackgroundColor) // <- сделано опциональным
        /** Текст под спиннером */
        SLATE_ATTRIBUTE(FText, LoadingText) // <- сделано опциональным
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    // SWidget interface
    virtual void Tick(const FGeometry& AllottedGeometry,
                      const double InCurrentTime,
                      const float InDeltaTime) override;

private:
    /** Угол вращения спиннера (обновляется в Tick) */
    float SpinnerAngle = 0.0f;

    /** Возвращает трансформацию для анимации спиннера */
    FSlateRenderTransform GetSpinnerTransform() const;
};
