#include "ItemPlacement/A_AreaDropZone.h"
#include "PythonContainers/A_InteractableActor.h"
#include "FloorAssignmentComponent.h"

AA_AreaDropZone::AA_AreaDropZone()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	DropZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DropZoneMesh"));
	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
	FloorAssignmentComponent->Registrate(EFloorActorType::HeavyFurniture, ESnapshotChannel::None);

	DropZoneMesh->SetupAttachment(RootComponent);

	DropZoneMesh->SetCollisionProfileName(TEXT("DropZoneObject"));
}

void AA_AreaDropZone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SetMeshMaterialAndState(0, IsPlacingOnScene);

	if (IsValid(ParentActor))
	{
		AttachToActor(ParentActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
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

void AA_AreaDropZone::UpdateInAreaItemsList()
{
	if (!bIsActive)
	{
		return;
	}

	CorrectItemsList.Empty();
	WrongItemsList.Empty();

	TArray <AActor*> OverlappingActors;
	DropZoneMesh->GetOverlappingActors(OverlappingActors);

	for (AActor* OverlappedActor : OverlappingActors)
	{
		if (AA_InteractableActor* InteractableActor = Cast<AA_InteractableActor>(OverlappedActor))
		{
			int32* Required = ItemsTags.Find(InteractableActor->Tag);
			int32& Correct = CorrectItemsList.FindOrAdd(InteractableActor->Tag);

			if (Required && Correct < *Required)
			{
				++Correct;
			}
			else
			{
				++WrongItemsList.FindOrAdd(InteractableActor->Tag);
			}
		}
	}
}