#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "ScannableActorData.generated.h"

USTRUCT(BlueprintType)
struct ALSEXTRAS_API FScannableActorData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	FString ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	FGameplayTag ItemTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	FGameplayTag ItemPropertyTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	FName DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	uint8 bCanBeScanned : 1{true};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	uint8 bHasBeenScanned : 1{false};

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "ScannableActor")
	uint8 bHasBeenBagged : 1{false};
};


USTRUCT(BlueprintType)
struct ALSEXTRAS_API FPropertyCompatibilityRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag PropertyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTagContainer IncompatibleProperties;
};
