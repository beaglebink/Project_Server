#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "O_RealEstateStructures.h"
#include "DA_RealEstateQuestionnaire.generated.h"

UCLASS()
class ALSEXTRAS_API UDA_RealEstateQuestionnaire : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstateQuestionnaire")
	TMap<FString, FQuestionDataStruct> Questions;
};
