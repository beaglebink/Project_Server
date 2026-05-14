#include "WireAndConnections/A_Wire.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

AA_Wire::AA_Wire()
{
	PrimaryActorTick.bCanEverTick = true;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));

	SplineComponent->SetupAttachment(RootComponent);
}

void AA_Wire::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SplineComponent->ClearSplinePoints();
	for (USplineMeshComponent* SplineMesh : SplineMeshComponents)
	{
		if (SplineMesh)
		{
			SplineMesh->DestroyComponent();
		}
	}
	SplineMeshComponents.Empty();

	if (WireMesh)
	{
		WireMeshLength = WireMesh->GetBounds().BoxExtent.X * 2;
		SplineComponent->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local);

		for (int32 i = 1; i <= NumberOfSegments; ++i)
		{
			FVector Dir = SplineComponent->GetRotationAtSplinePoint(i - 1, ESplineCoordinateSpace::Local).Vector().GetSafeNormal();
			FRotator RandomRot = FRotator(0.0f, FMath::RandRange(-PointNoise, PointNoise), 0.0f);
			FVector NoisyDir = RandomRot.RotateVector(Dir);
			FVector PointLocation = SplineComponent->GetLocationAtSplinePoint(i - 1, ESplineCoordinateSpace::Local) + NoisyDir * WireMeshLength;
			SplineComponent->AddSplinePoint(PointLocation, ESplineCoordinateSpace::Local);
		}

		for (int32 i = 1; i <= NumberOfSegments; ++i)
		{
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this);
			SplineMeshComponents.Add(SplineMesh);
			SplineMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			SplineMesh->SetForwardAxis(ESplineMeshAxis::X);
			SplineMesh->SetupAttachment(RootComponent);
			SplineMesh->RegisterComponent();
			SplineMesh->SetStaticMesh(WireMesh);
			SplineMesh->SetStartAndEnd(SplineComponent->GetLocationAtSplinePoint(i - 1, ESplineCoordinateSpace::World), SplineComponent->GetTangentAtSplinePoint(i - 1, ESplineCoordinateSpace::World),
				SplineComponent->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World), SplineComponent->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::World));
		}
	}
}

void AA_Wire::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Wire::BeginPlay()
{
	Super::BeginPlay();

}