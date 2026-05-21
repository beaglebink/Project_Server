#pragma once

#include "CoreMinimal.h"
#include "WireAndConnections/A_WireConnector.h"
#include "A_Wire.generated.h"

class UCableComponent;
class UPhysicsConstraintComponent;
class UStaticMesh;

UCLASS()
class ALSEXTRAS_API AA_Wire : public AA_WireConnector
{
	GENERATED_BODY()

public:
	AA_Wire();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMesh* OppositeConnectorMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UCableComponent* CableComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UPhysicsConstraintComponent* Constraint;
};
