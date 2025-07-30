#include "Inventory/UI/W_VisualDescription.h"
#include "Inventory/UI/Visual3D/A_3DDescription.h"
#include "Inventory/S_ItemData.h"
#include "Components/TextBlock.h"

void UW_VisualDescription::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (RenderActorClass)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		RenderActor = GetWorld()->SpawnActor<AA_3DDescription>(RenderActorClass, FVector(0.0f, 0.0f, -10000.0f), FRotator::ZeroRotator, Params);
		if (RenderActor)
		{
			if (UStaticMesh* MeshToRender = ItemDataTable->FindRow<FS_ItemData>(ItemName, TEXT("Find row in datatable"))->StaticMesh)
			{
				RenderActor->StaticMeshComponent->SetStaticMesh(MeshToRender);
			}
		}
	}
	TextBlock_Description->SetText(ItemDataTable->FindRow<FS_ItemData>(ItemName, TEXT("Find row in datatable"))->Description);
}

void UW_VisualDescription::NativeDestruct()
{
	Super::NativeDestruct();

	RenderActor->Destroy();
}
