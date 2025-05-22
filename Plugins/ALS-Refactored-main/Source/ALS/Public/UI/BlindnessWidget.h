#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BlindnessWidget.generated.h"

UCLASS()
class ALS_API UBlindnessWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* FadeOut;
};
