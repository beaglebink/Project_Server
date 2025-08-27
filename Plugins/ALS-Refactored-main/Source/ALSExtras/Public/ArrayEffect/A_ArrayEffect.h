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
	UChildActorComponent* AppendNodeComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AA_ArrayNode> NodeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* SwapAudioComp;

	AA_ArrayNode* AppendNode;

	TArray<AA_ArrayNode*> NodeArray;

	TArray<FVector> LocationArray;

	UFUNCTION()
	void AddNewNode();

	UFUNCTION()
	void DeleteNode(int32 Index);

public:
	uint8 bIsSwapping : 1{false};

	void SwapNode(int32 Node1, int32 Node2);

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
	virtual void TimelineProgress(float Value);

	UFUNCTION()
	virtual void TimelineFinished();
};
