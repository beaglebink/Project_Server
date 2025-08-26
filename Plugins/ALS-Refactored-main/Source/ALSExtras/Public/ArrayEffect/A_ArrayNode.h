#pragma once

#include "Components/TimelineComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ArrayNode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrab);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDelete, int32, Index);

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

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* NodeBorder;

	UPROPERTY()
	UPrimitiveComponent* GrabbedComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USoundBase* NodeMoveSound;

	int32 NodeIndex = -1;

	uint8 bIsOccupied : 1{false};

	uint8 bShouldGrab : 1{false};

	uint8 bIsMoveLeft : 1{false};

	uint8 bIsMoving : 1{false};

	FVector CurrentLocation;

	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* Other, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ComponentGrabbing();

	void DeleteNode();

public:
	float NodeBorderYSize;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_BorderMaterial;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getter")
	int32 GetIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Setter")
	void SetIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, Category = "Material")
	void SetBorderMaterialAndIndex(int32 NewIndex = -1);

	UFUNCTION(BlueprintCallable, Category = "ArrayInteraction")
	void GetTextCommand(FText Command);

	void MoveNode(bool Direction);

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Delegate")
	FOnGrab OnGrabDel;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "Delegate")
	FOnDelete OnDeleteDel;

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
