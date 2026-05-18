#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PythonContainers/A_InteractableActor.h"
#include "A_Wire.generated.h"

class UCableComponent;
class UPhysicsConstraintComponent;
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* MiddlePoint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* FemaleMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UCableComponent* CableComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UPhysicsConstraintComponent* ConstraintStartMiddle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UPhysicsConstraintComponent* ConstraintMiddleEnd;
};
