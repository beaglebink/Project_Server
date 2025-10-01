#pragma once

#include "ArrayEffect/A_PythonContainer.h"
#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "A_ArrayEffect.generated.h"

UCLASS()
class ALSEXTRAS_API AA_ArrayEffect : public AA_PythonContainer
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
	UAudioComponent* SwapAudioComp;

public:
	virtual void GetTextCommand(FText Command)override;

private:
	void SwapNodes(int32 Node1, int32 Node2);

	void InsertNode(FName VariableName, int32 Index);

	bool ParseCommandToSwap(FText Command, FText& PrevName, int32& OutIndex1, int32& OutIndex2);

	bool ParseCommandToInsert(FText Command, FText& PrevName, FName& VariableName, int32& OutIndex);

private:
	UPROPERTY(EditDefaultsOnly, Category = "Curve", meta = (AllowPrivateAccess = "true"))
	UCurveFloat* HeightFloatCurve;

	int32 SwapNode1;

	int32 SwapNode2;

protected:
	UPROPERTY()
	UTimelineComponent* SwapTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* FloatCurve;

	FOnTimelineFloat ProgressFunction;

	FOnTimelineEvent FinishedFunction;

	UFUNCTION()
	void SwapTimelineProgress(float Value);

	UFUNCTION()
	void SwapTimelineFinished();
};
