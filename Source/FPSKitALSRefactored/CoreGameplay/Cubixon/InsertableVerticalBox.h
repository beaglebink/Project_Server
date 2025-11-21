#pragma once

#include "CoreMinimal.h"
#include "Components/VerticalBox.h"
#include "InsertableVerticalBox.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UInsertableVerticalBox : public UVerticalBox
{
    GENERATED_BODY()

public:
    /** Вставить новый виджет на конкретный индекс */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void InsertWidgetAt(UWidget* NewWidget, int32 Index);

protected:
    /** Постоянный массив всех виджетов */
    UPROPERTY()
    TArray<UWidget*> Widgets;

    /** Инициализация массива при первом использовании */
    void InitializeWidgetsArray();
};