#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Scanning/I_ScannableObject.h"
#include "W_ScannedObjectInfo.generated.h"

UCLASS()
class ALSEXTRAS_API UW_ScannedObjectInfo : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanning", meta = (ExposeOnSpawn = "true"))
	TScriptInterface<II_ScannableObject> ScannedActor;
};
