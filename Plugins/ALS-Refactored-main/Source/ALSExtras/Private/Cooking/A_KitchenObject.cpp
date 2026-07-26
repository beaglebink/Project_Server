#include "Cooking/A_KitchenObject.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Cooking/A_Dishes.h"

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

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			TSet<AActor*> OverlappingActors;
			SurfaceCollisionComponent->GetOverlappingActors(OverlappingActors, AA_Dishes::StaticClass());

			for (AActor* OverlappedActor : OverlappingActors)
			{
				if (AA_Dishes* Dish = Cast<AA_Dishes>(OverlappedActor))
				{
					Dish->bIsPlacing = true;
				}
			}
		}, 0.5f, true);
}

void AA_KitchenObject::Destroyed()
{
	Super::Destroyed();

}
