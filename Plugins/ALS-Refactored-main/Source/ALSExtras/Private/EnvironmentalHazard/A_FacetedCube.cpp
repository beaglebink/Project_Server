#include "EnvironmentalHazard/A_FacetedCube.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

AA_FacetedCube::AA_FacetedCube()
{
	PrimaryActorTick.bCanEverTick = true;
	SideStateArray.Init(false, 6);
}

void AA_FacetedCube::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	MeshDynamicMaterialArray.Empty();

	for (size_t i = 0; i < 6; ++i)
	{
		MeshDynamicMaterialArray.Add(StaticMeshComp->CreateAndSetMaterialInstanceDynamic(i));
		if (MeshDynamicMaterialArray[i])
		{
			MeshDynamicMaterialArray[i]->SetScalarParameterValue("Index", SideStateArray[i]);
		}
	}
}

void AA_FacetedCube::BeginPlay()
{
	Super::BeginPlay();
}

void AA_FacetedCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_FacetedCube::HandleWeaponShot_Implementation(FHitResult& Hit)
{
	if (bIsOnDeath)
	{
		return;
	}

	AudioComp->Play();

	UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Hit.GetComponent());

	if (!MeshComp || !MeshComp->GetStaticMesh() || Hit.FaceIndex == -1)
	{
		return;
	}

	const FStaticMeshLODResources& LODResource = MeshComp->GetStaticMesh()->GetRenderData()->LODResources[0];
	const FIndexArrayView Indices = LODResource.IndexBuffer.GetArrayView();

	for (int32 SectionIndex = 0; SectionIndex < LODResource.Sections.Num(); ++SectionIndex)
	{
		const FStaticMeshSection& Section = LODResource.Sections[SectionIndex];

		if (Hit.FaceIndex >= (int32)Section.FirstIndex / 3 && Hit.FaceIndex < (int32)(Section.FirstIndex + Section.NumTriangles * 3) / 3)
		{
			SideStateArray[SectionIndex] = !SideStateArray[SectionIndex];
			MeshDynamicMaterialArray[SectionIndex]->SetScalarParameterValue("Index", SideStateArray[SectionIndex]);
		}
	}

	for (bool State : SideStateArray)
	{
		if (State)
		{
			return;
		}
	}

	OnDeath();
}

void AA_FacetedCube::OnMeshHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& HitResult)
{
	if (!bCanBeHit || bIsOnDeath || !OtherActor)
	{
		return;
	}

	Super::OnMeshHit(HitComp, OtherActor, OtherComp, NormalImpulse, HitResult);

	int32 DangerSidesQuantity = 0;
	for (bool SideState : SideStateArray)
	{
		DangerSidesQuantity += SideState;
	}

	UGameplayStatics::ApplyDamage(OtherActor, DamageCaused * DangerSidesQuantity, OtherActor->GetInstigatorController(), this, nullptr);
}
