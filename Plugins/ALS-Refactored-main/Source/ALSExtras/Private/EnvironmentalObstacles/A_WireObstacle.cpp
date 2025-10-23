#include "EnvironmentalObstacles/A_WireObstacle.h"
#include "Components/BoxComponent.h"
#include "Components/BillboardComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

AA_WireObstacle::AA_WireObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));

	BoxComponent->SetupAttachment(RootComponent);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
}

void AA_WireObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	//Build anchors
	LeftAnchors.Empty();
	RightAnchors.Empty();

	for (int32 i = 0; i < AnchorsPerSide; ++i)
	{
		float T = (float)i / (AnchorsPerSide - 1);

		float Angle = FMath::Lerp(-PI / 2.05f, PI / 2.05f, T);
		float Y = FMath::Cos(Angle) * WireObstacleRadius;
		float Z = FMath::Sin(Angle) * WireObstacleRadius;

		LeftAnchors.Add(GetActorLocation() + FVector(0, Y, Z));
		RightAnchors.Add(GetActorLocation() + FVector(0, -Y, Z));
	}

	//Find constraints points
	const float TraceSpread = 20.0f;

	LeftHitPoints.Empty();
	RightHitPoints.Empty();

	for (size_t i = 0; i < LeftAnchors.Num(); ++i)
	{
		FVector Offset = FVector(FMath::RandRange(-TraceSpread, TraceSpread), 0.0f, 0.0f);
		FHitResult Hit;
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(this);

		bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), LeftAnchors[i], ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Hit, true);

		if (bHit)
		{
			LeftHitPoints.Add(Hit.ImpactPoint);
		}
		bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), RightAnchors[i], ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Hit, true);

		if (bHit)
		{
			RightHitPoints.Add(Hit.ImpactPoint);
		}
	}

	if (!LeftHitPoints.IsEmpty() && !RightHitPoints.IsEmpty())
	{
		//Find boxcollision size
		float BoxWidth = 0.0f;
		float BoxHeight = 0.0f;
		FVector BoxLocation = GetActorLocation();
		for (size_t i = 0; i < FMath::Min(LeftHitPoints.Num(), RightHitPoints.Num()); ++i)
		{
			if (BoxWidth < FVector::Distance(LeftHitPoints[i], RightHitPoints[i]))
			{
				BoxWidth = FVector::Distance(LeftHitPoints[i], RightHitPoints[i]);
				BoxLocation.Y = (LeftHitPoints[i].Y + RightHitPoints[i].Y) / 2.0f;
			}
		}
		for (size_t i = 0, j = LeftHitPoints.Num() - 1; i < j; ++i, --j)
		{
			if (BoxHeight < FVector::Distance(LeftHitPoints[i], LeftHitPoints[j]))
			{
				BoxHeight = FVector::Distance(LeftHitPoints[i], LeftHitPoints[j]);
				BoxLocation.Z = (LeftHitPoints[i].Z + LeftHitPoints[j].Z) / 2.0f;
			}
		}
		for (size_t i = 0, j = RightHitPoints.Num() - 1; i < j; ++i, --j)
		{
			if (BoxHeight < FVector::Distance(RightHitPoints[i], RightHitPoints[j]))
			{
				BoxHeight = FVector::Distance(RightHitPoints[i], RightHitPoints[j]);
				BoxLocation.Z = (RightHitPoints[i].Z + RightHitPoints[j].Z) / 2.0f;
			}
		}
		BoxComponent->SetBoxExtent(FVector(4.0f, BoxWidth / 2.0f, BoxHeight / 2.0f));
		BoxComponent->SetWorldLocation(BoxLocation);

		//Build wires
		for (size_t Times = 0; Times < 2; ++Times)
		{
			ShuffleArray(LeftHitPoints);
			ShuffleArray(RightHitPoints);
			for (size_t i = 0; i < FMath::Min(LeftHitPoints.Num(), RightHitPoints.Num()); ++i)
			{
				DrawDebugLine(GetWorld(), LeftHitPoints[i], RightHitPoints[i], FColor::Black, false, 10.0f, 0u, 1.0f);
			}
		}
	}
}

void AA_WireObstacle::BeginPlay()
{
	Super::BeginPlay();
}

void AA_WireObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

template<typename T>
void AA_WireObstacle::ShuffleArray(TArray<T>& Array)
{
	for (int32 i = 0; i < Array.Num(); ++i)
	{
		int32 SwapIndex = FMath::RandRange(i, Array.Num() - 1);
		if (i != SwapIndex)
		{
			Array.Swap(i, SwapIndex);
		}
	}
}
