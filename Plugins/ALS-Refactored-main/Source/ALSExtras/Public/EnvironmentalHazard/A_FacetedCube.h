#pragma once

#include "CoreMinimal.h"
#include "EnvironmentalHazard/A_EnvironmentalHazard.h"
#include "A_FacetedCube.generated.h"

UCLASS()
class ALSEXTRAS_API AA_FacetedCube : public AA_EnvironmentalHazard
{
	GENERATED_BODY()

public:
	AA_FacetedCube();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components|Property", meta = (AllowPrivateAccess = true))
	TArray<bool> SideStateArray;

	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> MeshDynamicMaterialArray;

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;

	virtual void OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)override;
};
