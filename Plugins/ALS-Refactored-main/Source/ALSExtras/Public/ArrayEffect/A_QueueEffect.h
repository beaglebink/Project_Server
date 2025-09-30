#pragma once

#include "ArrayEffect/A_PythonContainer.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "A_QueueEffect.generated.h"

class AA_ArrayNode;
class UBoxComponent;
class UTextRenderComponent;

UCLASS()
class ALSEXTRAS_API AA_QueueEffect : public AA_PythonContainer
{
	GENERATED_BODY()
	
public:	
	AA_QueueEffect();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

public:
	TArray<AA_ArrayNode*> NodeArray;

	uint8 bIsOverlapping : 1{false};

	uint8 bIsSwapping : 1{false};

	uint8 bIsOnConcatenation : 1{false};

	uint8 bIsDetaching : 1{false};

	uint8 bIsAttaching : 1{false};

	int32 SizeOfConcatenatingContainer = -1;

	void GetTextCommand(FText Command);

public:
	void AppendNode(FName VariableName = NAME_None);

private:
	void SwapNodes(int32 Node1, int32 Node2);

	void DeleteNode(int32 Index);

	void InsertNode(FName VariableName, int32 Index);

	void ContainerPop(int32 Index);

	void ContainerClear();

	void ContainerExtend();

public:
	void ContainerConcatenate(AA_QueueEffect* ArrayToConcatenate);

private:
	void ContainerRename(FText NewName);

	void ContainerCopy(FText Name, int32 OutLeftIndex, int32 OutRightIndex);

private:
	bool ParseCommandToAppend(FText Command, FText& PrevName, FName& VariableName);

	bool ParseCommandToSwap(FText Command, FText& PrevName, int32& OutIndex1, int32& OutIndex2);

	bool ParseCommandToDelete(FText Command, FText& PrevName, int32& OutIndex, int32& OutLeftIndex, int32& OutRightIndex);

	bool ParseCommandToInsert(FText Command, FText& PrevName, FName& VariableName, int32& OutIndex);

	bool ParseCommandToPop(FText Command, FText& PrevName, FName& VariableName, int32& Index);

	bool ParseCommandToClear(FText Command, FText& PrevName);

	bool ParseCommandToExtend(FText Command, FText& PrevName, TArray<int32>& OutArray);

	bool ParseCommandToConcatenate(FText Command, int32& OutSize1, int32& OutSize2);

	bool ParseCommandToReset(FText Command, FText& PrevName);

	bool ParseCommandToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum);

	bool ParseCommandToCopy(FText Command, FText& PrevName, FText& CopyName, int32& OutLeftIndex, int32& OutRightIndex);

private:
	void AttachToCharacterCamera();

	void DetachFromCharacterCamera();

	void AttachToContainer();

	void MoveNodesConsideringOrder();

	void RefreshNameLocationAndRotation();

	void SetContainerName(FText Name);

	AActor* GetActorWithTag(const FName& Tag);

	UPROPERTY(EditDefaultsOnly, Category = "Curve", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* HeightFloatCurve;

	int32 SwapNode1;

	int32 SwapNode2;

	TArray<int32> ExtendContainer;

	int32 ExtendContainerIndex = 0;

protected:
	UPROPERTY()
	UTimelineComponent* SwapTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* FloatCurve;

	FOnTimelineFloat ProgressFunction;

	FOnTimelineEvent FinishedFunction;

	UFUNCTION()
	virtual void TimelineProgress(float Value);

	UFUNCTION()
	virtual void TimelineFinished();
};
