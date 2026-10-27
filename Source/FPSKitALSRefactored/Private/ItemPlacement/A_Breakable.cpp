#include "ItemPlacement/A_Breakable.h"
#include "FloorAssignmentComponent.h"
#include "Components/AudioComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"
#include "ItemPlacement/A_DropZone.h"

AA_Breakable::AA_Breakable()
{
	PrimaryActorTick.bCanEverTick = true;

	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	AudioComponent->SetupAttachment(RootComponent);

	StaticMesh->SetNotifyRigidBodyCollision(true);

	AudioComponent->SetAutoActivate(false);
}

void AA_Breakable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!HasAnyFlags(RF_ClassDefaultObject) && FloorAssignmentComponent && !FloorAssignmentComponent->GetItemId().IsValid())
	{
		FloorAssignmentComponent->Registrate(EFloorActorType::HeavyFurniture, ESnapshotChannel::None);
	}

	DefaultTransform = GetActorTransform();
}

void AA_Breakable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Breakable::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAnyFlags(RF_ClassDefaultObject) && FloorAssignmentComponent && !FloorAssignmentComponent->GetItemId().IsValid())
	{
		FloorAssignmentComponent->Registrate(EFloorActorType::HeavyFurniture, ESnapshotChannel::None);
	}

	StaticMesh->OnComponentHit.AddDynamic(this, &AA_Breakable::OnActorHit);
}

void AA_Breakable::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

}

void AA_Breakable::Destroyed()
{
	Super::Destroyed();
}

void AA_Breakable::BreakActor()
{
	SetIsBroken(true);

	AudioComponent->Play();

	if (!GeometryCollectionComponent && DefaultGeometryCollection)
	{
		GeometryCollectionComponent = NewObject<UGeometryCollectionComponent>(this, TEXT("GeometryCollectionComponent"));
		GeometryCollectionComponent->SetRestCollection(DefaultGeometryCollection);
		GeometryCollectionComponent->SetupAttachment(RootComponent);
		GeometryCollectionComponent->RegisterComponent();
		GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GeometryCollectionComponent->SetSimulatePhysics(true);
		GeometryCollectionComponent->SetVisibility(true);
		GeometryCollectionComponent->WakeAllRigidBodies();
		GeometryCollectionComponent->RemoveAllAnchors();
		GeometryCollectionComponent->CrumbleActiveClusters();
	}
}

void AA_Breakable::SetIsBroken(bool IsBroken)
{
	if (IsValid(GeometryCollectionComponent))
	{
		GeometryCollectionComponent->DestroyComponent();
		GeometryCollectionComponent = nullptr;
	}

	bIsBroken = IsBroken;

	if (bIsBroken)
	{
		StaticMesh->SetVisibility(false);
		StaticMesh->SetSimulatePhysics(false);
		StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (AttachingDropZone)
		{
			AttachingDropZone->bIsOccupied = false;
			AttachingDropZone = nullptr;
		}
	}
	else
	{
		StaticMesh->SetVisibility(true);
		StaticMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		if (AttachingDropZone)
		{
			StaticMesh->SetSimulatePhysics(false);
		}
		else
		{
			StaticMesh->SetSimulatePhysics(true);
		}
	}
}

bool AA_Breakable::GetIsBroken() const
{
	return bIsBroken;
}

void AA_Breakable::RestoreActor()
{
	SetIsBroken(false);

	SetActorTransform(DefaultTransform);
}

void AA_Breakable::OnActorHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsBroken)
	{
		return;
	}

	if (NormalImpulse.Size() >= BreakImpulseThreshold)
	{
		BreakActor();
	}
}
