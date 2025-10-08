#pragma once

#include "EnvironmentalHazard/A_EnvironmentalHazard.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "A_HazardActor.generated.h"

UCLASS()
class ALSEXTRAS_API AA_HazardActor : public AA_EnvironmentalHazard
{
	GENERATED_BODY()

public:
	AA_HazardActor();

protected:
	virtual void OnConstruction(const FTransform& Transform)override;

	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void HandleWeaponShot_Implementation(FHitResult& Hit)override;
};
