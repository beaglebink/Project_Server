#include "EnvironmentalHazard/A_HazardCube.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AlsCharacterExample.h"

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
		MeshDynamicMaterial->SetScalarParameterValue("Index", static_cast<int32>(DamageCaused));
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

	int32 DamageCausedLocal = static_cast<int32>(DamageCaused);
	--DamageCausedLocal;

	if (DamageCausedLocal >= 0)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Index", DamageCausedLocal);
	}

	if (DamageCausedLocal == 0)
	{
		OnDeath();
	}

	DamageCaused = DamageCausedLocal;
}

void AA_HazardCube::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!bCanBeHit || bIsOnDeath || !OtherActor)
	{
		return;
	}

	Super::OnMeshHit(HitComp, OtherActor, OtherComp, NormalImpulse, HitResult);

	if (AAlsCharacterExample* Player = Cast<AAlsCharacterExample>(OtherActor))
	{
		DamageMultiplier = 1.0f;
		if (Player->bCalculatorGogglesIsOn)
		{
			DamageMultiplier = 0.5f;
		}
	}

	UGameplayStatics::ApplyDamage(OtherActor, DamageCaused * DamageMultiplier, OtherActor->GetInstigatorController(), this, HazardDamageTypeClass);
}
