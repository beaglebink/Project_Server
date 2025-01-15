#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShareLibrary.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API UShareLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Asset Analysis")
    static void AnalyzeAssets();
};
