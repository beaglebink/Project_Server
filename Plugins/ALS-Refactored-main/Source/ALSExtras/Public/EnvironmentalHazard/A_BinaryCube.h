#pragma once

#include "CoreMinimal.h"
#include "EnvironmentalHazard/A_EnvironmentalHazard.h"
#include "A_BinaryCube.generated.h"

class UInteractiveItemComponent;
class UInteractivePickerComponent;

UCLASS()
class ALSEXTRAS_API AA_BinaryCube : public AA_EnvironmentalHazard
{
	GENERATED_BODY()
public:
	AA_BinaryCube();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = true))
	UInteractiveItemComponent* InteractiveComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Property", meta = (AllowPrivateAccess = true))
	uint8 bBinary : 1{false};

	uint8 bGrabOrDrop : 1{true};

	UInteractivePickerComponent* Grabber;

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)override;

	UFUNCTION()
	void GrabDropObject(UInteractivePickerComponent* Picker);

	void SetCubePhysic(bool IsSet);
};