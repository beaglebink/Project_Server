#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerController_I.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UPlayerController_I : public UInterface
{
	GENERATED_BODY()
};

class ALS_API IPlayerController_I
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SeamlessTravel")
	void SetLoadScreen(bool IsLoadScreen);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SeamlessTravel")
	void StoreWeaponState();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "SeamlessTravel")
	void RestoreWeaponState();
};