#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Cookable.generated.h"

class UProceduralMeshComponent;

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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Slicing mesh")
	UProceduralMeshComponent* SlicedMesh;
};
