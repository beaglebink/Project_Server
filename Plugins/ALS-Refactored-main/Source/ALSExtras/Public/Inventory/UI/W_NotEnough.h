#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_NotEnough.generated.h"

UCLASS()
class ALSEXTRAS_API UW_NotEnough : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, Category = "Animation", meta = (BindWidgetAnim), Transient)
	UWidgetAnimation* Appearing;

public:
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayAppearing();
};
