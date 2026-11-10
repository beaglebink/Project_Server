#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_RealEstateQuestionnaire.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAnswerSelected, FString, Question, int32, AnswerIndex, bool, IsSelected);

UCLASS()
class ALSEXTRAS_API UW_RealEstateQuestionnaire : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Questionnaire")
	FOnAnswerSelected OnAnswerSelected;

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Questionnaire")
	void UpdateQuestionsData(const TMap<FString, FQuestionDataStruct>& QuestionsData);
};
