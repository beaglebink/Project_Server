#pragma once

#include "CoreMinimal.h"
#include "ChoreSystem/ChoreEnums.h"
#include "GameplayTagContainer.h"
#include "O_QuestStructures.generated.h"

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

USTRUCT(BlueprintType)
struct FQuestStepDataStruct
{
	GENERATED_BODY()

	UPROPERTY()
	EChoreStatus QuestStatus = EChoreStatus::Inactive;

	UPROPERTY()
	TArray<FQuestStepDataArray> QuestsArray;
};