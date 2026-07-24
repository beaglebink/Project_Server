#include "Cooking/A_KitchenObject.h"
#include "Components/BoxComponent.h"

AA_KitchenObject::AA_KitchenObject()
{
	PrimaryActorTick.bCanEverTick = true;

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	SurfaceCollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("SurfaceCollisionComponent"));

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent->SetupAttachment(RootComponent);
	SurfaceCollisionComponent->SetupAttachment(StaticMeshComponent);

	SurfaceCollisionComponent->SetBoxExtent(FVector(100.0f, 100.0f, 10.0f));
}

void AA_KitchenObject::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_KitchenObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_KitchenObject::BeginPlay()
{
	Super::BeginPlay();

}

void AA_KitchenObject::Destroyed()
{
	Super::Destroyed();

}
