#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_BagDropZone.generated.h"

class USphereComponent;

UCLASS()
class ALSEXTRAS_API AA_BagDropZone : public AA_InteractableActor
{
	GENERATED_BODY()

public:
	AA_BagDropZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "DropZone|ItemCollision")
	USphereComponent* ItemSphereCollision;

	UPROPERTY()
	UStaticMeshComponent* BagDropZoneMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropZone|ItemMaterial")
	UMaterialInterface* BagDropZoneMeshOverlayMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshOverlayMaterial;

	void SetMeshMaterialAndState(int32 NewState, bool IsPlacing);

private:
	uint8 IsPlacingOnScene : 1{true};
};
