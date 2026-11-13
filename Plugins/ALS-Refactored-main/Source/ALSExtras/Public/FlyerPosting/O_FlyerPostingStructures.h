#pragma once

#include "CoreMinimal.h"
#include "O_FlyerPostingStructures.generated.h"

USTRUCT(BlueprintType)
struct FFlyerPostingStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	FText AnswerText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	uint8 bIsSuitable : 1{false};


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	uint8 bIsChecked : 1{false};
};