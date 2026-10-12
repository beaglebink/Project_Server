#include "ItemPlacement/A_ChorePlacementZone.h"
#include "ItemPlacement/A_DropZone.h"
#include "ItemPlacement/A_AreaDropZone.h"
#include "PythonContainers/A_InteractableActor.h"

AA_ChorePlacementZone::AA_ChorePlacementZone()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AA_ChorePlacementZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// AreaDropZone actor
	if (!AreaDropZone && AreaDropZoneClass)
	{
		AreaDropZone = GetWorld()->SpawnActor<AA_AreaDropZone>(AreaDropZoneClass, GetActorTransform());
	}

	if (AreaDropZone)
	{
		AreaDropZone->SetActorTransform(GetActorTransform());
	}

	// DropZones actors
	if (DropZoneClass)
	{
		for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
		{
			if (DropZonePlacementZone.ItemClass)
			{
				if (AA_InteractableActor* ItemCDO = DropZonePlacementZone.ItemClass->GetDefaultObject<AA_InteractableActor>())
				{
					if (ItemCDO->StaticMesh)
					{
						DropZonePlacementZone.DropZoneMeshSize = ItemCDO->StaticMesh->Bounds.BoxExtent * 2.0f;
						DropZonePlacementZone.DropZoneMesh->SetRelativeScale3D(DropZonePlacementZone.DropZoneMeshSize / 90.f);
					}
				}

				while (DropZonePlacementZone.DropZones.Num() < DropZonePlacementZone.DropZoneQuantity)
				{
					AA_DropZone* DropZone = GetWorld()->SpawnActor<AA_DropZone>(DropZoneClass, GetActorTransform());
					DropZone->ItemClass = DropZonePlacementZone.ItemClass;
					DropZonePlacementZone.DropZones.Add(DropZone);
				}
			}
			while (DropZonePlacementZone.DropZones.Num() > DropZonePlacementZone.DropZoneQuantity)
			{
				DropZonePlacementZone.DropZones.Pop()->Destroy();
			}
		}
	}
}

void AA_ChorePlacementZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_ChorePlacementZone::BeginPlay()
{
	Super::BeginPlay();

}

void AA_ChorePlacementZone::Destroyed()
{
	Super::Destroyed();

	if (AreaDropZone)
	{
		AreaDropZone->Destroy();
	}
}
