#pragma once

#include "CoreMinimal.h"
#include "ChoreSystem/ChoreEnums.h"
#include "GameplayTagContainer.h"
#include "O_QuestStructures.generated.h"

USTRUCT(BlueprintType)
struct FQuestPartStepData
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
struct FQuestPartData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 bIsCompleted : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestPartStepData> QuestPartStepDataArray;
};

USTRUCT(BlueprintType)
struct FQuestData
{
	GENERATED_BODY()

	UPROPERTY()
	EChoreStatus QuestStatus = EChoreStatus::Inactive;

	UPROPERTY()
	uint8 bWasStarted : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FQuestPartData> QuestPartsArray;
};

USTRUCT(BlueprintType)
struct FQuestSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FQuestData> QuestDataMap;
};