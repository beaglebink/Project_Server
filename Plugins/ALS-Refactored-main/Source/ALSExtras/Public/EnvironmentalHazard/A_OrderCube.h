#pragma once

#include "EnvironmentalHazard/A_EnvironmentalHazard.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_OrderCube.generated.h"

UCLASS()
class ALSEXTRAS_API AA_OrderCube : public AA_EnvironmentalHazard
{
	GENERATED_BODY()
	
public:	
	AA_OrderCube();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY()
	int32 CubeIndex = -1;

private:
	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)override;
};