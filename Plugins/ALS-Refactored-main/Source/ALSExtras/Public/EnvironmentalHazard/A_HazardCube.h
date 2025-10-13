#pragma once

#include "CoreMinimal.h"
#include "EnvironmentalHazard/A_EnvironmentalHazard.h"
#include "A_HazardCube.generated.h"

UCLASS()
class ALSEXTRAS_API AA_HazardCube : public AA_EnvironmentalHazard
{
	GENERATED_BODY()

public:
	AA_HazardCube();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)override;
};
