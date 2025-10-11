#include "EnvironmentalHazard/A_OrderCube.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnvironmentalHazard/A_OrderSystem.h"

AA_OrderCube::AA_OrderCube()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_OrderCube::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComp->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial)
	{
		MeshDynamicMaterial->SetScalarParameterValue("Index", CubeIndex);
	}
}

void AA_OrderCube::BeginPlay()
{
	Super::BeginPlay();
}

void AA_OrderCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_OrderCube::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	if (this == Hit.GetActor())
	{
		if (AA_OrderSystem* OrderSystem = Cast<AA_OrderSystem>(GetOwner()))
		{
			OrderSystem->CheckHitCubeRightOrderIndex(this);
		}
	}
}

void AA_OrderCube::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!bCanBeHit || bIsOnDeath || !OtherActor)
	{
		return;
	}

	Super::OnMeshHit(HitComp, OtherActor, OtherComp, NormalImpulse, HitResult);

	UGameplayStatics::ApplyDamage(OtherActor, DamageCaused, OtherActor->GetInstigatorController(), this, nullptr);
}
