#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "Components/TimelineComponent.h"
#include "A_Cookable.generated.h"

class UProceduralMeshComponent;
class AA_Dishes;

UCLASS()
class ALSEXTRAS_API AA_Cookable : public AA_InteractableActor
{
	GENERATED_BODY()

public:
	AA_Cookable();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	virtual void HandleCutting_Implementation(UPARAM(ref)FHitResult& Hit, FVector CutPlaneNormal) override;

	void CopyProceduralMesh(UProceduralMeshComponent* Source, UProceduralMeshComponent* Target);

	void BuildConvexCollision(UProceduralMeshComponent* Mesh);

public:
	void Toss(float Delta);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slicing mesh")
	UProceduralMeshComponent* SlicedMesh;

	UPROPERTY()
	AA_Dishes* AttachedDish;

	uint8 bIsAttaching : 1{false};

private:
	float OnAttachingPauseCheckTime = 0.0f;

	FVector SavedLocalPosition;

	FRotator TossStartRotation;
	FRotator TossTargetRotation;

	float TossOfset = 0.0f;

protected:
	//Toss Timeline
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UTimelineComponent* TossTimeline;

	UPROPERTY(EditAnywhere, Category = "Components|Timeline")
	UCurveFloat* TossFloatCurve;

	FOnTimelineFloat TossProgressFunction;

	FOnTimelineEvent TossFinishedFunction;

	UFUNCTION()
	void TossTimelineProgress(float Value);

	UFUNCTION()
	void TossTimelineFinished();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cooking")
	int32 CookingTime;
};
