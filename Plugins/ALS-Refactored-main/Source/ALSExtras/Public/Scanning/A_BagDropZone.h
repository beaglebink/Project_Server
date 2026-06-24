#pragma once

#include "CoreMinimal.h"
#include "PythonContainers/A_InteractableActor.h"
#include "DA_PropertyCompatibility.h"
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
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Bag|Collision")
	USphereComponent* ItemSphereCollision;

	UPROPERTY()
	UMaterialInstanceDynamic* DMI_MeshMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Bag")
	UDA_PropertyCompatibility* CompatibilityData;

	void SetMeshMaterialAndState(int32 NewState, bool IsPlacing);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bag|PackedItems")
	TArray<AA_InteractableActor*> PackedItems;

	UFUNCTION(BlueprintCallable, Category = "Bag|PackedItems")
	void PackItemIntoBag(AA_InteractableActor* Item);

	UFUNCTION(BlueprintCallable, Category = "Bag|PackedItems")
	bool CheckItemProperty(AA_InteractableActor* Item);
};
