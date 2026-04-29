#include "ItemPlacement/A_DropZone.h"
#include "Components/SphereComponent.h"
#include "PythonContainers/A_InteractableActor.h"

AA_DropZone::AA_DropZone()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	DropZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DropZoneMesh"));
	ItemSphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ItemSphereCollision"));

	DropZoneMesh->SetupAttachment(RootComponent);
	ItemSphereCollision->SetupAttachment(RootComponent);

	DropZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemSphereCollision->SetCollisionProfileName(TEXT("DropZoneObject"));
}

void AA_DropZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ItemClass)
	{
		if (AA_InteractableActor* Item = ItemClass->GetDefaultObject<AA_InteractableActor>())
		{
			const UStaticMeshComponent* SourceMesh = Item->FindComponentByClass<UStaticMeshComponent>();
			if (SourceMesh && SourceMesh->GetStaticMesh())
			{
				DropZoneMesh->SetStaticMesh(SourceMesh->GetStaticMesh());
			}
		}
	}

	if (DropZoneMesh)
	{
		ItemSphereCollision->SetSphereRadius(DropZoneMesh->Bounds.SphereRadius);
	}

	SetMeshMaterialAndState(0, IsPlacingOnScene);
}


void AA_DropZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_DropZone::BeginPlay()
{
	Super::BeginPlay();

	IsPlacingOnScene = false;
	SetMeshMaterialAndState(0, IsPlacingOnScene);
}

void AA_DropZone::SetMeshMaterialAndState(int32 NewState, bool IsPlacing)
{
	if (DropZoneMesh && DropZoneMeshMaterial)
	{
		if (!DMI_MeshMaterial)
		{
			DMI_MeshMaterial = UMaterialInstanceDynamic::Create(DropZoneMeshMaterial, this);
			if (DMI_MeshMaterial)
			{
				DropZoneMesh->SetMaterial(0, DMI_MeshMaterial);
				DMI_MeshMaterial->SetScalarParameterValue(FName("State"), NewState);
				DMI_MeshMaterial->SetScalarParameterValue(FName("IsPlacingOnScene"), IsPlacingOnScene ? 1.0f : 0.0f);
			}
		}
		else
		{
			DMI_MeshMaterial->SetScalarParameterValue(FName("State"), NewState);
			DMI_MeshMaterial->SetScalarParameterValue(FName("IsPlacingOnScene"), IsPlacingOnScene ? 1.0f : 0.0f);
		}
	}
}