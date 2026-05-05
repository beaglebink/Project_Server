#pragma once

#include "CoreMinimal.h"
#include "ProxyFunctionCall.generated.h"

USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FProxyFunctionCall
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName FunctionName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Args;
};
