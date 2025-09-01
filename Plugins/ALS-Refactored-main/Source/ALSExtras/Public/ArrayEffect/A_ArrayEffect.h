#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ArrayEffect.generated.h"

class AA_ArrayNode;

UCLASS()
class ALSEXTRAS_API AA_ArrayEffect : public AActor
{
	GENERATED_BODY()

public:
	AA_ArrayEffect();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* EndNodeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AA_ArrayNode> NodeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* SwapAudioComp;

	FVector DefaultLocation;

	AA_ArrayNode* EndNode;
	
	float NodeWidth;

	TArray<FVector> LocationArray;

public:
	TArray<AA_ArrayNode*> NodeArray;

	uint8 bIsOverlapping : 1{false};

	uint8 bIsSwapping : 1{false};

	uint8 bIsOnConcatenation : 1{false};

	void GetTextCommand(FText Command);

private:
	UFUNCTION()
	void AppendNode();

	void SwapNodes(int32 Node1, int32 Node2);

	void DeleteNode(int32 Index);

	void InsertNode(int32 Index);

	void ArrayPop();

	void ArrayClear();

	void ArrayExtend();

	void ArrayConcatenate();

	bool ParseArrayIndexToAppend(FText Command);

	bool ParseArrayIndexToSwap(FText Command, int32& OutIndex1, int32& OutIndex2);

	bool ParseArrayIndexToDel(FText Command, int32& OutIndex);

	bool ParseArrayIndexToInsert(FText Command, int32& OutIndex);

	bool ParseArrayIndexToPop(FText Command);

	bool ParseArrayIndexToClear(FText Command);

	bool ParseArrayIndexToExtend(FText Command, TArray<int32>& OutArray);

	bool ParseArrayIndexToConcatenate(FText Command, int32& OutSize1, int32& OutSize2);

	void AttachToCharacterCamera();

	UPROPERTY(EditDefaultsOnly, Category = "Curve", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* HeightFloatCurve;

	int32 SwapNode1;

	int32 SwapNode2;

	TArray<int32> ExtendArray;

	int32 ExtendArrayIndex = 0;

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
