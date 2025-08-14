#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "TeleportFailResponseObject.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UTeleportFailResponseObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString ObjectId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString DestinationId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Response;
};