#pragma once

#include "CoreMinimal.h"
#include "ProxyProperty.generated.h"

USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FProxyProperty
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName Name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Value;
};
