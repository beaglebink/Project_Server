#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ScannableActorData.h"
#include "DA_PropertyCompatibility.generated.h"

UCLASS()
class ALSEXTRAS_API UDA_PropertyCompatibility : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FPropertyCompatibilityRule> CompatibilityRules;

	UFUNCTION(BlueprintCallable, Category = "PropertyCompatibility")
	bool GetRule(const FGameplayTag& PropertyTag, FPropertyCompatibilityRule& OutRule) const;

};
