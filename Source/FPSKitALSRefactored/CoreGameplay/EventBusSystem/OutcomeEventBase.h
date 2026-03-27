#pragma once
#include "CoreMinimal.h"
#include "OutcomeEventBase.generated.h"

USTRUCT(BlueprintType)
struct FOutcomeEventBase
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString OutcomeType; 
};
