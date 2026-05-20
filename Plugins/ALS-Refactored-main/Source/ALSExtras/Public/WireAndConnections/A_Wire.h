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

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UStaticMeshComponent* FemaleMeshComponent;

	UFUNCTION(BlueprintCallable, Category = "Wire")
	void SetConnectorType(bool bIsMale);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wire")
	bool GetConnectorType(UStaticMeshComponent*& ConnectorMesh) const { ConnectorMesh = CurrentConnectorMeshComponent; return bIsMaleUsed; }

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Wire")
	void OnConnectorTypeChanged(bool bIsMale);

	UFUNCTION(BlueprintCallable, Category = "Wire")
	void SetPower(bool OnPower);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Wire")
	bool GetPower() const { return bIsOnPower; }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Wire")
	void OnPowerChanged(bool OnPower);

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UCableComponent* CableComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UPhysicsConstraintComponent* Constraint;

	UPROPERTY(VisibleDefaultsOnly, Category = "Wire")
	UStaticMeshComponent* CurrentConnectorMeshComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Wire")
	uint8 bIsMaleUsed : 1{true};

	UPROPERTY(VisibleDefaultsOnly, Category = "Wire")
	uint8 bIsOnPower : 1{false};
};
