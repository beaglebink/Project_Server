#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/I_PowerConnection.h"
#include "A_DropZone.generated.h"

class AA_InteractableActor;
class USphereComponent;

UCLASS()
class FPSKITALSREFACTORED_API AA_DropZone : public AActor, public II_PowerConnection
{
	GENERATED_BODY()
	
public:	
	AA_DropZone();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|ParentActor")
	AActor* ParentActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|Item")
	TSubclassOf<AA_InteractableActor> ItemClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DropZone|WireConnector")
	FString WirePlugInConnectorType{""};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DropZone|WireConnector")
	uint8 bIsPowerOn : 1{false};

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "DropZone|ItemCollision")
	USphereComponent* ItemSphereCollision;

	UPROPERTY()
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

	virtual void OnPowerConnected_Implementation(bool IsConnected) override;

private:
	uint8 IsPlacingOnScene : 1{true};
};
