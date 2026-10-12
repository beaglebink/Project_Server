#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_ChorePlacementZone.generated.h"

class AA_DropZone;
class AA_AreaDropZone;
class AA_InteractableActor;

USTRUCT(BlueprintType)
struct FDropZonePlacementZone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_InteractableActor> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones", meta = (ClampMin = 0))
	int32 DropZoneQuantity;

	TArray<AA_DropZone*> DropZones;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|AreaMesh")
	UStaticMeshComponent* DropZoneMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|AreaMeshMaterial")
	UMaterialInterface* DropZoneMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|AreaMeshSize")
	FVector DropZoneMeshSize;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;
};

UCLASS()
class FPSKITALSREFACTORED_API AA_ChorePlacementZone : public AActor
{
	GENERATED_BODY()

public:
	AA_ChorePlacementZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_AreaDropZone> AreaDropZoneClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TSubclassOf<AA_DropZone> DropZoneClass;

	UPROPERTY(BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	AA_AreaDropZone* AreaDropZone;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ChorePlacement|DropZones")
	TArray<FDropZonePlacementZone> DropZonePlacementZones;

};
