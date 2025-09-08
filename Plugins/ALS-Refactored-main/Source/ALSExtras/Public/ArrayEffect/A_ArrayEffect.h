#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ArrayEffect.generated.h"

class AA_ArrayNode;
class UBoxComponent;
class UTextRenderComponent;

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
	UBoxComponent* CollisionComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UChildActorComponent* EndNodeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AA_ArrayNode> NodeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AA_ArrayEffect> ArrayClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* SwapAudioComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UTextRenderComponent* TextComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	FText ArrayName;

	FVector DefaultLocation;

	FRotator DefaultRotation;

	AA_ArrayNode* EndNode;

	float NodeLength;

	float NodeWidth;

	float NodeHigh;

public:
	TArray<AA_ArrayNode*> NodeArray;

	uint8 bIsOverlapping : 1{false};

	uint8 bIsSwapping : 1{false};

	uint8 bIsOnConcatenation : 1{false};

	uint8 bIsDetaching : 1{false};

	uint8 bIsAttaching : 1{false};

	int32 SizeOfConcatenatingArray = -1;

	void GetTextCommand(FText Command);

public:
	void AppendNode();

private:
	void SwapNodes(int32 Node1, int32 Node2);

	void DeleteNode(int32 Index);

	void InsertNode(int32 Index);

	void ArrayPop();

	void ArrayClear();

	void ArrayExtend();

public:
	void ArrayConcatenate(AA_ArrayEffect* ArrayToConcatenate);

private:
	void ArraySplit(int32 SplitIndex, bool MoveDirection);

	void ArrayRename(FText NewName);

	void ArrayCopy(FText Name);

private:
	bool IsValidPythonIdentifier(const FString& Str);

	bool ParseCommandToAppend(FText Command);

	bool ParseCommandToSwap(FText Command, int32& OutIndex1, int32& OutIndex2);

	bool ParseCommandToDel(FText Command, int32& OutIndex);

	bool ParseCommandToInsert(FText Command, int32& OutIndex);

	bool ParseCommandToPop(FText Command);

	bool ParseCommandToClear(FText Command);

	bool ParseCommandToExtend(FText Command, TArray<int32>& OutArray);

	bool ParseCommandToConcatenate(FText Command, int32& OutSize1, int32& OutSize2);

	bool ParseCommandToSplitUndestructive(FText Command, int32& OutIndex, bool& Direction);

	bool ParseCommandToRename(FText Command, FText& PrevName, FText& NewName, int32& ArrayNum);

	bool ParseCommandToCopy(FText Command, FText& Name, FText& CopyName);

private:
	void AttachToCharacterCamera();

	void DetachFromCharacterCamera();

	void AttachToArray();

	void MoveArrayOnSplit(AA_ArrayEffect* ArrayToMove, bool Direction);

	void RefreshNameLocationAndRotation();

	void SetArrayName(FText Name);

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
