#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_WeaponInteraction.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UI_WeaponInteraction : public UInterface
{
    GENERATED_BODY()
};

class ALSEXTRAS_API II_WeaponInteraction
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WeaponInterface")
    void HandleWeaponShot();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "WeaponInterface")
    void HandleTextFromWeapon();
};