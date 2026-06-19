#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Scanning/ScannableActorData.h"
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
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scanning")
	FScannableActorData GetScannableObjectInfo(AActor* ScannableActor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scanning")
	void SetScannableObjectInfo(const FScannableActorData& NewData);
};