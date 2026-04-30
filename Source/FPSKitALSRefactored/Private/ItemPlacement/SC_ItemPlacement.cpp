#include "ItemPlacement/SC_ItemPlacement.h"
#include "Components/SphereComponent.h"
#include "ItemPlacement/A_DropZone.h"
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

	DropZoneSearcherSphere->SetSphereRadius(500.0f);
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
		DropZone->SetMeshMaterialAndState(1, false);
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
}

void USC_ItemPlacement::OnCheckerSphereOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || ItemName.IsNone())
	{
		return;
	}

	if (AA_DropZone* DropZone = Cast<AA_DropZone>(OtherActor))
	{
		if (!DropZone->bIsOccupied && DropZone->ItemName == ItemName)
		{
			ChoosenDropZone = DropZone;
			ChoosenDropZone->SetMeshMaterialAndState(3, false);
		}
		else
		{
			DropZone->SetMeshMaterialAndState(2, false);
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
		if (!DropZone->bIsOccupied)
		{
			DropZone->SetMeshMaterialAndState(1, false);
			ChoosenDropZone = nullptr;
		}
	}
}

void USC_ItemPlacement::GrabItemToDropZone(AA_InteractableActor* Item)
{
	if (!ChoosenDropZone)
	{
		return;
	}

	if (UStaticMeshComponent* StaticMeshComponent = Item->FindComponentByClass<UStaticMeshComponent>())
	{
		StaticMeshComponent->SetSimulatePhysics(false);
		Item->AttachToActor(ChoosenDropZone, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ChoosenDropZone->bIsOccupied = true;
		ChoosenDropZone->SetMeshMaterialAndState(0, false);
	}
}
