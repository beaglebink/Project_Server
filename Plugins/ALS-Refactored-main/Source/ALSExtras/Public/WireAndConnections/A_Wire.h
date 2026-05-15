#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Wire.generated.h"

class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;

UCLASS()
class ALSEXTRAS_API AA_Wire : public AA_InteractableActor
{
	GENERATED_BODY()

public:
	AA_Wire();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	USplineComponent* SplineComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMesh* WireMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	int32 NumberOfSegments = 20;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
	float PointNoise = 10.0f;

	float WireMeshLength;

	TArray<USplineMeshComponent*> SplineMeshComponents;
};
