// CheckResponsePayload.h
#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "CheckResponsePayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UCheckResponsePayload : public UOutcomePayload
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, Category = "Check")
    FGuid TransactionId;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    bool bApproved = false;

    UPROPERTY(BlueprintReadWrite, Category = "Check")
    FString Reason;
};