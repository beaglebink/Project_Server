#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "I_SaveableObject.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UI_SaveableObject : public UInterface
{
	GENERATED_BODY()
};

class FPSKITALSREFACTORED_API II_SaveableObject
{
	GENERATED_BODY()

public:
	// Text file (and heirs) save data collection and application functions
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	TMap<FString, FText> CollectTextFilesSaveData();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveObject")
	void ApplyTextFilesSaveData(const TMap<FString, FText>& SaveData);


};
