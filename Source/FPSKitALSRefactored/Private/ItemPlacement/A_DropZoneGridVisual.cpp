#include "ItemPlacement/A_DropZoneGridVisual.h"
#include "ItemPlacement/A_ChorePlacementZone.h"
#include "ItemPlacement/A_DropZone.h"

AA_DropZoneGridVisual::AA_DropZoneGridVisual()
{
	PrimaryActorTick.bCanEverTick = true;

	DropZoneMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DropZoneMeshComponent"));

	RootComponent = DropZoneMeshComponent;
	DropZoneMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AA_DropZoneGridVisual::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (DropZoneMeshMaterial)
	{
		if (!DMI_MeshMaterial)
		{
			DMI_MeshMaterial = DropZoneMeshComponent->CreateDynamicMaterialInstance(0, DropZoneMeshMaterial);
		}
		else
		{
			DropZoneMeshComponent->SetMaterial(0, DMI_MeshMaterial);
		}
	}

	if (IsValid(ParentActor))
	{
		AttachToActor(ParentActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else if (GetOwner())
	{
		AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);
	}
}

void AA_DropZoneGridVisual::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_DropZoneGridVisual::BeginPlay()
{
	Super::BeginPlay();

	if (DMI_MeshMaterial)
	{
		DMI_MeshMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.0f);
	}
}

void AA_DropZoneGridVisual::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyDropZoneChildren();

	Super::EndPlay(EndPlayReason);
}

void AA_DropZoneGridVisual::Destroyed()
{
	DestroyDropZoneChildren();

	Super::Destroyed();
}

void AA_DropZoneGridVisual::AddChildDropZone(AA_DropZone* DropZone)
{
	DropZoneChildren.AddUnique(DropZone);
}

TArray<AA_DropZone*> AA_DropZoneGridVisual::GetDropZoneChildren() const
{
	return DropZoneChildren;
}

void AA_DropZoneGridVisual::DestroyDropZoneChildren()
{
	for (AA_DropZone* DropZoneToDestroy : DropZoneChildren)
	{
		if (IsValid(DropZoneToDestroy))
		{
			DropZoneToDestroy->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			DropZoneToDestroy->Destroy();
		}
	}
	DropZoneChildren.Empty();
}

#if WITH_EDITOR
void AA_DropZoneGridVisual::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (AA_ChorePlacementZone* OwnerZone = Cast<AA_ChorePlacementZone>(GetOwner()))
	{
		OwnerZone->RerunConstructionScripts();
	}
}
#endif