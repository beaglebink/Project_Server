#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CoreBlueprintFunctionLibrary.h"
#include "DA_FluidPoints.generated.h"

UCLASS(BlueprintType, Blueprintable)
class ALSEXTRAS_API UDA_FluidPoints : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Fluid Points")
	FFluidPoints FluidPoints;
};
