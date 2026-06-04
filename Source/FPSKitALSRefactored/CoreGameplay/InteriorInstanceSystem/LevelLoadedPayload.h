#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "LevelLoadedPayload.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API ULevelLoadedPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Level")
    FString LevelName;

    UPROPERTY(BlueprintReadWrite, Category = "Level")
    FString LevelPackageName;

    UFUNCTION(BlueprintCallable, Category = "Level")
    ULevelLoadedPayload* Setup(const FString& InLevelName, const FString& InPackageName)
    {
        LevelName = InLevelName;
        LevelPackageName = InPackageName;
        return this;
    }
};