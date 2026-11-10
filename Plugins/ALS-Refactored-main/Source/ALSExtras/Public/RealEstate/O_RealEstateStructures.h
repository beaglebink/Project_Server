#pragma once

#include "CoreMinimal.h"
#include "O_RealEstateStructures.generated.h"

USTRUCT(BlueprintType)
struct FAnswerDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	FText AnswerText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	uint8 bIsSuitable : 1{false};


	UPROPERTY()
	uint8 bIsChecked : 1{false};
};

USTRUCT(BlueprintType)
struct FQuestionDataStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	TArray<FAnswerDataStruct> AnswersArray;

	UPROPERTY()
	uint8 bIsPassed : 1{false};
};