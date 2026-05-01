#include "ItemPlacement/A_AreaDropZone.h"
#include "PythonContainers/A_InteractableActor.h"

AA_AreaDropZone::AA_AreaDropZone()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	DropZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DropZoneMesh"));

	DropZoneMesh->SetupAttachment(RootComponent);

	DropZoneMesh->SetCollisionProfileName(TEXT("DropZoneObject"));
}

void AA_AreaDropZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (ItemsClasses.Num() > 0)
	{
		for (const TSubclassOf<AA_InteractableActor>& ItemClass : ItemsClasses)
		{
			if (!ItemClass)
			{
				continue;
			}

			if (AA_InteractableActor* Item = ItemClass->GetDefaultObject<AA_InteractableActor>())
			{
				ItemsNames.Add(Item->Name);
			}
		}
	}

	DropZoneMesh->SetWorldScale3D(DropZoneMeshSize / 100.0f);
	SetMeshMaterialAndState(0, IsPlacingOnScene);
}

void AA_AreaDropZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_AreaDropZone::BeginPlay()
{
	Super::BeginPlay();

	IsPlacingOnScene = false;
	SetMeshMaterialAndState(0, IsPlacingOnScene);
}

void AA_AreaDropZone::SetMeshMaterialAndState(int32 NewState, bool IsPlacing)
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