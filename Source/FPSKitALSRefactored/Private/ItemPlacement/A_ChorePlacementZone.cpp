#include "ItemPlacement/A_ChorePlacementZone.h"
#include "ItemPlacement/A_DropZone.h"
#include "ItemPlacement/A_AreaDropZone.h"
#include "ItemPlacement/A_DropZoneGridVisual.h"
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
		AreaDropZone->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	// DropZones actors
	if (DropZoneClass && DropZoneGridVisualClass)
	{
		for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
		{
			// Create the DropZoneGridVisual actor for this placement zone
			FVector DropZonesGridStartLocation{ 0.0f,0.0f,0.0f };
			if (!DropZonePlacementZone.DropZoneGridVisual)
			{
				FActorSpawnParameters Params;
				Params.Owner = this;
				DropZonePlacementZone.DropZoneGridVisual = GetWorld()->SpawnActor<AA_DropZoneGridVisual>(DropZoneGridVisualClass, GetActorTransform(), Params);
				DropZonePlacementZone.DropZoneGridVisual->AttachToActor(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			}
			else if (DropZonePlacementZone.DropZoneGridVisual->GetRootComponent())
			{
				DropZonesGridStartLocation = DropZonePlacementZone.DropZoneGridVisual->GetDropZoneGridVisualStartLocation();
			}

			// Count the number of items that can fit in the drop zone based on the item mesh size and the drop zone mesh size
			if (DropZonePlacementZone.ItemClass)
			{
				if (AA_InteractableActor* ItemCDO = DropZonePlacementZone.ItemClass->GetDefaultObject<AA_InteractableActor>())
				{
					if (ItemCDO->StaticMesh && ItemCDO->StaticMesh->GetStaticMesh() && DropZonePlacementZone.DropZoneGridVisual->GetRootComponent())
					{
						DropZonePlacementZone.ItemMeshSize = ItemCDO->StaticMesh->GetStaticMesh()->GetBounds().BoxExtent * 2.2f;

						FVector DropZoneMeshSize = DropZonePlacementZone.DropZoneGridVisual->GetRootComponent()->Bounds.BoxExtent * 2.0f;

						DropZonePlacementZone.ItemsRows = FMath::FloorToInt(DropZoneMeshSize.X / DropZonePlacementZone.ItemMeshSize.X);
						DropZonePlacementZone.ItemsColumns = FMath::FloorToInt(DropZoneMeshSize.Y / DropZonePlacementZone.ItemMeshSize.Y);
						DropZonePlacementZone.DropZoneQuantity = DropZonePlacementZone.ItemsRows * DropZonePlacementZone.ItemsColumns;
					}
				}
				DropZonesGridStartLocation += FVector(DropZonePlacementZone.ItemMeshSize.X / 2.0f, -DropZonePlacementZone.ItemMeshSize.Y / 2.0f, 0.0f); // Adjust the starting location to center the grid of drop zones within the drop zone mesh

				while (DropZonePlacementZone.DropZones.Num() < DropZonePlacementZone.DropZoneQuantity)
				{
					AA_DropZone* DropZone = GetWorld()->SpawnActorDeferred<AA_DropZone>(DropZoneClass, GetActorTransform(), this);
					DropZone->ItemClass = DropZonePlacementZone.ItemClass;
					DropZone->FinishSpawning(GetActorTransform());
					DropZone->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
					DropZonePlacementZone.DropZones.Add(DropZone);
				}
			}
			while (DropZonePlacementZone.DropZones.Num() > DropZonePlacementZone.DropZoneQuantity)
			{
				AA_DropZone* ZoneToDestroy = DropZonePlacementZone.DropZones.Pop();
				ZoneToDestroy->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				ZoneToDestroy->Destroy();
			}

			// Arrange the drop zones in a grid based on the number of rows and columns
			for (int32 Row = 0; Row < DropZonePlacementZone.ItemsRows; ++Row)
			{
				for (int32 Column = 0; Column < DropZonePlacementZone.ItemsColumns; ++Column)
				{
					int32 Index = Row * DropZonePlacementZone.ItemsColumns + Column;
					if (DropZonePlacementZone.DropZones.IsValidIndex(Index))
					{
						FVector LocationOffset = FVector(Row * DropZonePlacementZone.ItemMeshSize.X, -Column * DropZonePlacementZone.ItemMeshSize.Y, 0.0f);
						DropZonePlacementZone.DropZones[Index]->SetActorLocation(DropZonesGridStartLocation + LocationOffset);
					}
				}
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
	if (AreaDropZone)
	{
		AreaDropZone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		AreaDropZone->Destroy();
	}

	for (FDropZonePlacementZone& DropZonePlacementZone : DropZonePlacementZones)
	{
		if (DropZonePlacementZone.DropZoneGridVisual)
		{
			DropZonePlacementZone.DropZoneGridVisual->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			DropZonePlacementZone.DropZoneGridVisual->Destroy();
		}

		for (AA_DropZone* DropZone : DropZonePlacementZone.DropZones)
		{
			if (DropZone)
			{
				DropZone->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				DropZone->Destroy();
			}
		}
		DropZonePlacementZone.DropZones.Empty();
	}

	Super::Destroyed();
}
