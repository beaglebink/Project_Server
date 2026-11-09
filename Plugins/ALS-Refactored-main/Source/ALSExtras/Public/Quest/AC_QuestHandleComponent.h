#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SaveSubsystems/I_SaveableObject.h"
#include "Quest/PDA_Quest.h"
#include "AC_QuestHandleComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSEXTRAS_API UAC_QuestHandleComponent : public UActorComponent, public II_SaveableObject
{
	GENERATED_BODY()

public:
	UAC_QuestHandleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "QuestData")
	TMap<FName, FQuestStepDataStruct> QuestDataMap;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool LoadQuestData(FName ChoreID);

	UFUNCTION(BlueprintCallable, Category = "ChoreStatus")
	void SetChoreStatus(FName ChoreID, EChoreStatus NewStatus);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ChoreStatus")
	EChoreStatus GetChoreStatus(FName ChoreID)const;

	// Quest data saving and loading
	virtual TMap<FName, FQuestStepDataStruct> CollectQuestSaveData_Implementation()override;

	virtual void ApplyQuestSaveData_Implementation(UGIS_Quest* GIS)override;

	// Intermediate data saving
	UFUNCTION(BlueprintCallable, Category = "Saving")
	FString CollectIntermediateSaveData(FName ChoreID);

	UFUNCTION(BlueprintCallable, Category = "Saving")
	void ApplyIntermediateSaveData(FName ChoreID, FString Jsonstring);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool CheckIfQuestIsCompleted(FName ChoreID);
};
