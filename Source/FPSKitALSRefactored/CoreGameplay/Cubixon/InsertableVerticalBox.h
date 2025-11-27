#pragma once

#include "CoreMinimal.h"
#include "Components/VerticalBox.h"
#include "InsertableVerticalBox.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UInsertableVerticalBox : public UVerticalBox
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "UI")
    void InsertWidgetAt(UWidget* NewWidget, int32 Index);
    virtual void ClearChildren() override;


protected:

    UPROPERTY()
    TArray<UWidget*> Widgets;

    void InitializeWidgetsArray();
};