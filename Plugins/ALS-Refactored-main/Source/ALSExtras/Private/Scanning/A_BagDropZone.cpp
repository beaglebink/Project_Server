#include "Scanning/A_BagDropZone.h"

AA_BagDropZone::AA_BagDropZone()
{

}

void AA_BagDropZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_BagDropZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_BagDropZone::BeginPlay()
{
	Super::BeginPlay();

}
void AA_BagDropZone::SetMeshMaterialAndState(int32 NewState, bool IsPlacing)
{
	if (BagDropZoneMesh && BagDropZoneMeshOverlayMaterial)
	{
		if (!DMI_MeshOverlayMaterial)
		{
			DMI_MeshOverlayMaterial = UMaterialInstanceDynamic::Create(BagDropZoneMeshOverlayMaterial, this);
			if (DMI_MeshOverlayMaterial)
			{
				BagDropZoneMesh->SetMaterial(0, DMI_MeshOverlayMaterial);
				DMI_MeshOverlayMaterial->SetScalarParameterValue(FName("State"), NewState);
				DMI_MeshOverlayMaterial->SetScalarParameterValue(FName("IsPlacingOnScene"), IsPlacingOnScene ? 1.0f : 0.0f);
			}
		}
		else
		{
			DMI_MeshOverlayMaterial->SetScalarParameterValue(FName("State"), NewState);
			DMI_MeshOverlayMaterial->SetScalarParameterValue(FName("IsPlacingOnScene"), IsPlacingOnScene ? 1.0f : 0.0f);
		}
	}
}