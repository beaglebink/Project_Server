#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerController_I.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPlayerController_I : public UInterface
{
	GENERATED_BODY()
};

class FPSKITALSREFACTORED_API IPlayerController_I
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "PlayerController")
	void SetLoadScreen(bool IsLoadScreen);
};