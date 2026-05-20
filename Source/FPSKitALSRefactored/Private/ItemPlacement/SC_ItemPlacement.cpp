#include "ItemPlacement/SC_ItemPlacement.h"
#include "Components/SphereComponent.h"
#include "ItemPlacement/A_DropZone.h"
#include "ItemPlacement/A_AreaDropZone.h"
#include "PythonContainers/A_InteractableActor.h"

USC_ItemPlacement::USC_ItemPlacement()
{
	PrimaryComponentTick.bCanEverTick = true;

	DropZoneSearcherSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DropZoneSearcherSphere"));
	DropZoneCheckerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DropZoneCheckerSphere"));

	DropZoneSearcherSphere->SetupAttachment(this);
	DropZoneCheckerSphere->SetupAttachment(this);

	DropZoneSearcherSphere->SetCollisionProfileName(TEXT("DropZoneInteractObject"));
	DropZoneCheckerSphere->SetCollisionProfileName(TEXT("DropZoneInteractObject"));
}

void USC_ItemPlacement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USC_ItemPlacement::BeginPlay()
{
	Super::BeginPlay();

	DropZoneSearcherSphere->OnComponentBeginOverlap.AddDynamic(this, &USC_ItemPlacement::OnSearcherSphereOverlapBegin);
	DropZoneSearcherSphere->OnComponentEndOverlap.AddDynamic(this, &USC_ItemPlacement::OnSearcherSphereOverlapEnd);
	DropZoneCheckerSphere->OnComponentBeginOverlap.AddDynamic(this, &USC_ItemPlacement::OnCheckerSphereOverlapBegin);
	DropZoneCheckerSphere->OnComponentEndOverlap.AddDynamic(this, &USC_ItemPlacement::OnCheckerSphereOverlapEnd);
}

void USC_ItemPlacement::OnSearcherSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	if (AA_DropZone* DropZone = Cast<AA_DropZone>(OtherActor))
	{
		if (!DropZone->bIsOccupied)
		{
			DropZone->SetMeshMaterialAndState(1, false);
		}
	}

	if (AA_AreaDropZone* AreaDropZone = Cast<AA_AreaDropZone>(OtherActor))
	{
		AreaDropZone->SetMeshMaterialAndState(1, false);
	}
}

void USC_ItemPlacement::OnSearcherSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	if (AA_DropZone* DropZone = Cast<AA_DropZone>(OtherActor))
	{
		DropZone->SetMeshMaterialAndState(0, false);
	}

	if (AA_AreaDropZone* AreaDropZone = Cast<AA_AreaDropZone>(OtherActor))
	{
		AreaDropZone->SetMeshMaterialAndState(0, false);
	}
}

void USC_ItemPlacement::OnCheckerSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || ItemName.IsNone())
	{
		return;
	}

	if (AA_DropZone* DropZone = Cast<AA_DropZone>(OtherActor))
	{
		if (!DropZone->bIsOccupied)
		{
			if (DropZone->ItemName == ItemName)
			{
				ChoosenDropZones.Add(DropZone);
				DropZone->SetMeshMaterialAndState(3, false);
			}
			else
			{
				DropZone->SetMeshMaterialAndState(2, false);
			}
		}
	}

	if (AA_AreaDropZone* AreaDropZone = Cast<AA_AreaDropZone>(OtherActor))
	{
		if (AreaDropZone->ItemsNames.Find(ItemName) != INDEX_NONE)
		{
			AreaDropZone->SetMeshMaterialAndState(3, false);
		}
		else
		{
			AreaDropZone->SetMeshMaterialAndState(2, false);
		}
	}
}

void USC_ItemPlacement::OnCheckerSphereOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor)
	{
		return;
	}

	if (AA_DropZone* DropZone = Cast<AA_DropZone>(OtherActor))
	{
		ChoosenDropZones.Remove(DropZone);
		if (!DropZone->bIsOccupied)
		{
			DropZone->SetMeshMaterialAndState(1, false);
		}
	}

	if (AA_AreaDropZone* AreaDropZone = Cast<AA_AreaDropZone>(OtherActor))
	{
		AreaDropZone->SetMeshMaterialAndState(1, false);
	}
}

void USC_ItemPlacement::AttachReleasedItemToDropZone(AA_InteractableActor* Item, bool bIsAttaching)
{
	if (bIsAttaching)
	{
		if (ChoosenDropZones.Num() > 0)
		{
			float DistanceToClosestDropZone = TNumericLimits<float>::Max();
			AA_DropZone* ChoosenDropZone = nullptr;

			for (AA_DropZone* DropZone : ChoosenDropZones)
			{
				float DistanceToDropZone = FVector::Dist(DropZone->GetActorLocation(), Item->GetActorLocation());
				if (DistanceToDropZone < DistanceToClosestDropZone)
				{
					DistanceToClosestDropZone = DistanceToDropZone;
					ChoosenDropZone = DropZone;
				}
			}

			if (UStaticMeshComponent* StaticMeshComponent = Item->FindComponentByClass<UStaticMeshComponent>())
			{
				StaticMeshComponent->SetSimulatePhysics(false);
				Item->AttachToActor(ChoosenDropZone, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				Item->AttachingDropZone = ChoosenDropZone;
				ChoosenDropZone->bIsOccupied = true;
				ChoosenDropZone->SetMeshMaterialAndState(0, false);
			}
		}
	}
	else
	{
		if (Item->AttachingDropZone)
		{
			if (UStaticMeshComponent* StaticMeshComponent = Item->FindComponentByClass<UStaticMeshComponent>())
			{
				StaticMeshComponent->SetSimulatePhysics(true);
				Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
				Item->AttachingDropZone->bIsOccupied = false;
				Item->AttachingDropZone->SetMeshMaterialAndState(3, false);
				Item->AttachingDropZone = nullptr;
			}
		}
	}
}
