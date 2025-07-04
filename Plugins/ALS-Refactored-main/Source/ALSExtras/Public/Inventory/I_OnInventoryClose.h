#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_OnInventoryClose.generated.h"

UINTERFACE()
class ALSEXTRAS_API UI_OnInventoryClose:public UInterface
{
	GENERATED_BODY()
};

class II_OnInventoryClose
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "InventoryInteraction")
	void OnCloseInventoryEvent();
};