#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ArrayNode.generated.h"

class AA_PythonContainer;

UCLASS()
class ALSEXTRAS_API AA_ArrayNode : public AActor
{
	GENERATED_BODY()

public:
	AA_ArrayNode();

protected:
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* NodeBorder;

	UPROPERTY()
	AActor* GrabbedActor;

	UPROPERTY()
	UPrimitiveComponent* GrabbedComponent;

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UAudioComponent* NodeBorderAudioComp;
	
	FVector CurrentLocation;

	FVector TargetLocation;

	int32 NodeIndex = -1;

	uint8 bIsOccupied : 1{false};

	uint8 bShouldGrab : 1{false};

public:
	FVector DefaultLocation;

	uint8 bIsMoving : 1{false};

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

public:
	void TryGrabActor(AActor* OtherActor);

private:
	void ComponentGrabbing();

public:
	void DeleteNode();

	UPROPERTY()
	AA_PythonContainer* OwnerActor;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_BorderMaterial;

	int32 GetIndex() const;

	void SetIndex(int32 NewIndex);

	void SetBorderMaterialAndIndex(int32 NewIndex = -1);
	
	void SetBorderMaterialIfOccupied(UPrimitiveComponent* Occupant);

	UFUNCTION(BlueprintCallable, Category = "ArrayInteraction")
	void GetTextCommand(FText Command);

	void AttachToNode(AActor* OtherActor);

	void DetachFromNode(AActor* OtherActor);

	void MoveNode(FVector TargetLocation);

protected:
	UPROPERTY()
	UTimelineComponent* MoveTimeline;

	UPROPERTY(EditDefaultsOnly, Category = "Timeline")
	UCurveFloat* FloatCurve;

	FOnTimelineFloat ProgressFunction;

	FOnTimelineEvent FinishedFunction;

	UFUNCTION()
	virtual void TimelineProgress(float Value);

	UFUNCTION()
	virtual void TimelineFinished();
};
