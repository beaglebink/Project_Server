#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "O_QuestStructures.h"
#include "PDA_Quest.generated.h"

UCLASS()
class ALSEXTRAS_API UPDA_Quest : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestStepDataArray> QuestsArray;
};
