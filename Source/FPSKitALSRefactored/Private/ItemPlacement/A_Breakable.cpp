#include "ItemPlacement/A_Breakable.h"
#include "GeometryCollection/GeometryCollectionComponent.h"

AA_Breakable::AA_Breakable()
{
	PrimaryActorTick.bCanEverTick = true;

	GeometryCollectionComponent = CreateDefaultSubobject<UGeometryCollectionComponent>(TEXT("GeometryCollectionComponent"));

	GeometryCollectionComponent->SetupAttachment(RootComponent);
	GeometryCollectionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GeometryCollectionComponent->SetVisibility(false);

	StaticMesh->SetNotifyRigidBodyCollision(true);
}

void AA_Breakable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_Breakable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Breakable::BeginPlay()
{
	Super::BeginPlay();

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

	GeometryCollectionComponent->SetSimulatePhysics(true);
	GeometryCollectionComponent->SetVisibility(true);
	GeometryCollectionComponent->WakeAllRigidBodies();
	GeometryCollectionComponent->AddRadialImpulse(GetActorLocation(), 100.0f, 500.0f, ERadialImpulseFalloff::RIF_Linear);
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
