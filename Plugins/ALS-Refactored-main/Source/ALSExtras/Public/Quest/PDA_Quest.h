#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PDA_Quest.generated.h"

USTRUCT(BlueprintType)
struct FQuestStepData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FGameplayTag QuestStepTag = FGameplayTag::EmptyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 QuestStepValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	uint8 bIsDynamic : 1{false};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	uint8 bShouldBeRemovedFromInventoryOnComplete : 1{false};

	UPROPERTY()
	uint8 bIsChecked : 1{false};
};

USTRUCT(BlueprintType)
struct FQuestStepDataArray
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 bIsCompleted : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestStepData> QuestStepDataArray;
};

UCLASS()
class ALSEXTRAS_API UPDA_Quest : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestStepDataArray> QuestsArray;
};
