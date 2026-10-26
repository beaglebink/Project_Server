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

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Destroyed() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|ParentActor")
	AActor* ParentActor;

	UPROPERTY(EditAnywhere, Category = "DropZone|AreaMesh")
	UStaticMeshComponent* DropZoneMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|AreaMeshMaterial")
	UMaterialInterface* DropZoneMeshMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;

private:
	UPROPERTY()
	TArray<AA_DropZone*> DropZoneChildren;

public:
	UFUNCTION(BlueprintCallable, Category = "DropZones")
	TArray<AA_DropZone*>& GetDropZoneChildren();

	UFUNCTION(BlueprintCallable, Category = "DropZones")
	void DestroyDropZoneChildren();

private:
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
#endif
};
