#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_DropZone.generated.h"

class AA_InteractableActor;
class USphereComponent;

UCLASS()
class FPSKITALSREFACTORED_API AA_DropZone : public AActor
{
	GENERATED_BODY()
	
public:	
	AA_DropZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|Item")
	TSubclassOf<AA_InteractableActor> ItemClass;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "DropZone|ItemCollision")
	USphereComponent* ItemSphereCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DropZone|ItemMesh")
	UStaticMeshComponent* DropZoneMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DropZone|ItemMaterial")
	UMaterialInterface* DropZoneMeshMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;

	UPROPERTY()
	FName ItemName;

	UPROPERTY()
	uint8 bIsOccupied : 1{false};

	void SetMeshMaterialAndState(int32 NewState, bool IsPlacing);

private:
	uint8 IsPlacingOnScene : 1{true};
};
