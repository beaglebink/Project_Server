#include "Scanning/A_BagDropZone.h"
#include "Components/SphereComponent.h"

AA_BagDropZone::AA_BagDropZone()
{
	ItemSphereCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ItemSphereCollision"));

	ItemSphereCollision->SetupAttachment(RootComponent);

	ItemSphereCollision->SetCollisionProfileName(TEXT("DropZoneObject"));
	ItemSphereCollision->SetSphereRadius(20.0f);
	ItemSphereCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
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
	if (StaticMesh)
	{
		if (!DMI_MeshMaterial)
		{
			DMI_MeshMaterial = UMaterialInstanceDynamic::Create(StaticMesh->GetMaterial(0), this);
			if (DMI_MeshMaterial)
			{
				StaticMesh->SetMaterial(0, DMI_MeshMaterial);
				DMI_MeshMaterial->SetScalarParameterValue(FName("State"), NewState);
			}
		}
		else
		{
			DMI_MeshMaterial->SetScalarParameterValue(FName("State"), NewState);
		}
	}
}

void AA_BagDropZone::PackItemIntoBag(AA_InteractableActor* Item)
{
	if (Item->Implements<UInteractiveActorInterface>())
	{
		if (UStaticMeshComponent* StaticMeshComponent = IInteractiveActorInterface::Execute_GetMeshComponent(Item))
		{
			StaticMeshComponent->SetSimulatePhysics(false);
			StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			StaticMeshComponent->SetVisibility(false);
			PackedItems.Add(Item);
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Packing item: %s into bag drop zone: %s"), *Item->GetName(), *GetName()));
}

bool AA_BagDropZone::CheckItemProperty(AA_InteractableActor* Item)
{
	if (Item->Implements<UI_ScannableObject>())
	{
		FScannableActorData HoldItemScannableData = II_ScannableObject::Execute_GetScannableObjectInfo(Item);
		if (!HoldItemScannableData.bHasBeenScanned || !HoldItemScannableData.bHasBeenBagged)
		{
			return false;
		}

		for (AA_InteractableActor* PackedItem : PackedItems)
		{
			if (PackedItem->Implements<UI_ScannableObject>())
			{
				FScannableActorData PackedItemScannableData = II_ScannableObject::Execute_GetScannableObjectInfo(PackedItem);
				FPropertyCompatibilityRule Rule;

				ensureMsgf(CompatibilityData->GetRule(PackedItemScannableData.ItemPropertyTag, Rule), TEXT("Missing compatibility rule for %s"), *PackedItemScannableData.ItemPropertyTag.ToString());

				if (Rule.IncompatibleProperties.HasTag(HoldItemScannableData.ItemPropertyTag))
				{
					return false;
				}
			}
		}
	}

	return true;
}
