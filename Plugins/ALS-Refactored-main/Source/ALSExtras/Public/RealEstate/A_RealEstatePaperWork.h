#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "O_RealEstateStructures.h"
#include "A_RealEstatePaperWork.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UFloorAssignmentComponent;
class UInteractiveItemComponent;
class UInteractivePickerComponent;
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
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	TSubclassOf<UW_RealEstateQuestionnaire> QuestionnaireWidgetClass;

	UPROPERTY()
	TObjectPtr<UW_RealEstateQuestionnaire> QuestionnaireWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UFloorAssignmentComponent* FloorAssignmentComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractiveItemComponent* InteractiveItemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UDA_RealEstateQuestionnaire* QuestionnaireDataAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RealEstate")
	TMap<FString, FQuestionDataStruct> QuestionsDataMap;

private:
	UPROPERTY()
	EChoreStatus ChoreStatus = EChoreStatus::Inactive;

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "RealEstate")
	bool CheckIfCompleted();

	UFUNCTION()
	void OnInteract(UInteractivePickerComponent* Picker);

	UFUNCTION()
	void OnInteractiveReceiveFocus(AActor* WhoInteract);

	void UpdateWidgetData();

public:
	UFUNCTION(BlueprintCallable, Category = "ChoreStatus")
	void SetChoreStatus(EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChoreStatus")
	EChoreStatus GetChoreStatus()const;

	UFUNCTION(BlueprintImplementableEvent, Category = "RealEstate")
	void OnStarted();

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
