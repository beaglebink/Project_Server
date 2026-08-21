#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_PourEvent.generated.h"

UCLASS()
class ALSEXTRAS_API UW_PourEvent : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "PourEvent")
	void UpdateVisualDataOnPourEvent(ELiquidType LiquidType, float PourAmount, float UnderAmountTolerance, float IdealMinimumAmount, float IdealMaximumAmount, float OverAmountTolerance);
};
