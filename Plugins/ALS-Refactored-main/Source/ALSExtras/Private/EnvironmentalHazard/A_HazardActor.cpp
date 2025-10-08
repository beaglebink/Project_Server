#include "EnvironmentalHazard/A_HazardActor.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AA_HazardActor::AA_HazardActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_HazardActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_HazardActor::BeginPlay()
{
	Super::BeginPlay();
}

void AA_HazardActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_HazardActor::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	AudioComp->Play();
	--DamageCaused;

	if (DamageCaused >= 0)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Damage", DamageCaused);
	}

	if (DamageCaused == 0)
	{
		OnDeath();
	}
}