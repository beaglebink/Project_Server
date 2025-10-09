#include "EnvironmentalHazard/A_HazardCube.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AA_HazardCube::AA_HazardCube()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_HazardCube::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComp->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Index", DamageCaused);
	}
}

void AA_HazardCube::BeginPlay()
{
	Super::BeginPlay();
}

void AA_HazardCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_HazardCube::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	if (bIsOnDeath)
	{
		return;
	}

	AudioComp->Play();
	--DamageCaused;

	if (DamageCaused >= 0)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Index", DamageCaused);
	}

	if (DamageCaused == 0)
	{
		OnDeath();
	}
}

void AA_HazardCube::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!bCanBeHit || bIsOnDeath || !OtherActor)
	{
		return;
	}

	Super::OnMeshHit(HitComp, OtherActor, OtherComp, NormalImpulse, HitResult);

	UGameplayStatics::ApplyDamage(OtherActor, DamageCaused, OtherActor->GetInstigatorController(), this, nullptr);
}
