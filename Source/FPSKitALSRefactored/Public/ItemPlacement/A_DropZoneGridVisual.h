#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_DropZoneGridVisual.generated.h"

UCLASS()
class FPSKITALSREFACTORED_API AA_DropZoneGridVisual : public AActor
{
	GENERATED_BODY()

public:
	AA_DropZoneGridVisual();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, Category = "DropZone|AreaMesh")
	UStaticMeshComponent* DropZoneMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|AreaMeshMaterial")
	UMaterialInterface* DropZoneMeshMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;

	FVector GetDropZoneGridVisualStartLocation() const;

private:
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif
};
