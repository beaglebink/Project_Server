#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_ScannableObject.generated.h"

UINTERFACE(BlueprintType)
class ALSEXTRAS_API UI_ScannableObject : public UInterface
{
	GENERATED_BODY()

};

class II_ScannableObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "InventoryInteraction")
	void GetScannableObjectInfo();
};