#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_PowerConnection.generated.h"


UINTERFACE(MinimalAPI, BlueprintType)
class UI_PowerConnection : public UInterface
{
	GENERATED_BODY()
};

class ALSEXTRAS_API II_PowerConnection
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PowerConnection")
	void OnPowerConnected(bool IsConnected);
};