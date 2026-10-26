#include "ItemPlacement/A_Breakable.h"
#include "FloorAssignmentComponent.h"
#include "Components/AudioComponent.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

AA_Breakable::AA_Breakable()
{
	PrimaryActorTick.bCanEverTick = true;

	FloorAssignmentComponent = CreateDefaultSubobject<UFloorAssignmentComponent>(TEXT("FloorAssignmentComponent"));
	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollectionComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));

	GeometryCollectionComponent->SetupAttachment(RootComponent);
	AudioComponent->SetupAttachment(RootComponent);

	StaticMesh->SetNotifyRigidBodyCollision(true);

	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GeometryCollectionComponent->SetSimulatePhysics(false);
	GeometryCollectionComponent->SetVisibility(false);

	AudioComponent->SetAutoActivate(false);
}

void AA_Breakable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!HasAnyFlags(RF_ClassDefaultObject) && FloorAssignmentComponent && !FloorAssignmentComponent->GetItemId().IsValid())
	{
		FloorAssignmentComponent->Registrate(EFloorActorType::HeavyFurniture, ESnapshotChannel::None);
	}
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
	bIsBroken = true;

	StaticMesh->SetSimulatePhysics(false);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMesh->SetVisibility(false);

	AudioComponent->Play();

	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GeometryCollectionComponent->SetSimulatePhysics(true);
	GeometryCollectionComponent->SetVisibility(true);
	GeometryCollectionComponent->WakeAllRigidBodies();
	GeometryCollectionComponent->WakeAllRigidBodies();
	GeometryCollectionComponent->RemoveAllAnchors();
	GeometryCollectionComponent->CrumbleActiveClusters();
}

void AA_Breakable::RestoreActor()
{
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
