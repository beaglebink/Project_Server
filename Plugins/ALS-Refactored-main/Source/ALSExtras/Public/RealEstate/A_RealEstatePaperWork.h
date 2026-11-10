#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "O_RealEstateStructures.h"
#include "A_RealEstatePaperWork.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UFloorAssignmentComponent;
class UDA_RealEstateQuestionnaire;
class UW_RealEstateQuestionnaire;

UCLASS()
class ALSEXTRAS_API AA_RealEstatePaperWork : public AActor, public II_SaveableObject
{
	GENERATED_BODY()
	
public:	
	AA_RealEstatePaperWork();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RealEstate")
	FName ChoreId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstate")
	uint8 bIsDeferredEvaluate : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstate", meta = (ClampMin = 0, ClampMax = 100))
	int32 RightAnswersPercentage = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstate")
	FGameplayTag CompletedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RealEstate")
	FGameplayTag FailedTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TriggerBoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TSubclassOf<UW_RealEstateQuestionnaire> QuestionnaireWidgetClass;

	UPROPERTY()
	TObjectPtr<UW_RealEstateQuestionnaire> QuestionnaireWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UDA_RealEstateQuestionnaire* QuestionnaireDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealEstate")
	TMap<FString, FQuestionDataStruct> QuestionsDataMap;

private:
	UPROPERTY()
	EChoreStatus ChoreStatus = EChoreStatus::Inactive;

	UFUNCTION()
	bool CheckIfCompleted();

	void OnBoxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);	

	void UpdateWidgetData();

public:
	UFUNCTION(BlueprintCallable, Category = "ChoreStatus")
	void SetChoreStatus(EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChoreStatus")
	EChoreStatus GetChoreStatus()const;

	UFUNCTION()
	void HandleAnswerSelected(FString Question, int32 AnswerIndex, bool IsSelected);

	// II_SaveableObject interface
	virtual TMap<FString, FQuestionDataStruct> CollectRealEstatePaperWorkSaveData_Implementation() override;

	virtual void ApplyRealEstatePaperWorkSaveData_Implementation(UGIS_RealEstatePaperWork* GIS) override;

	// Intermediate data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectIntermediateSaveData();

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplyIntermediateSaveData(FString Jsonstring);
};
