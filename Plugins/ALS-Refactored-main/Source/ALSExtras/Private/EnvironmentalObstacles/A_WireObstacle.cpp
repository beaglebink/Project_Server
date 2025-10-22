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
	BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	float WireRadius = 150.0f;
	const int32 TotalPerSide = 50;

	for (int32 i = 0; i < TotalPerSide; ++i)
	{
		FString Name = FString::Printf(TEXT("LeftBillboard_%d"), i);
		UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>(*Name);
		Billboard->SetupAttachment(RootComponent);
		LeftBillboards.Add(Billboard);
	}

	for (int32 i = 0; i < TotalPerSide; ++i)
	{
		FString Name = FString::Printf(TEXT("RightBillboard_%d"), i);
		UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>(*Name);
		Billboard->SetupAttachment(RootComponent);
		RightBillboards.Add(Billboard);
	}

	for (int32 i = 0; i < TotalPerSide; ++i)
	{
		float T = (float)i / (TotalPerSide - 1);

		float Angle = FMath::Lerp(-PI / 2.05f, PI / 2.05f, T);
		float Y = FMath::Cos(Angle) * WireRadius;
		float Z = FMath::Sin(Angle) * WireRadius;

		LeftBillboards[i]->SetWorldLocation(GetActorLocation() + FVector(0, Y, Z));
		RightBillboards[i]->SetWorldLocation(GetActorLocation() + FVector(0, -Y, Z));
	}
}

void AA_WireObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	//Build anchors
	for (int32 i = 0; i < 50; ++i)
	{
		LeftBillboards[i]->SetSprite(BillboardTexture);
		RightBillboards[i]->SetSprite(BillboardTexture);
	}

	//Find constraints points
	const float TraceSpread = 20.0f;
	const float TraceLength = 250.0f;

	LeftHitPoints.Empty();
	RightHitPoints.Empty();

	for (size_t i = 0; i < LeftBillboards.Num(); ++i)
	{
		FVector Offset = FVector(FMath::RandRange(-TraceSpread, TraceSpread), 0.0f, 0.0f);
		FHitResult Hit;
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(this);

		bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), GetActorLocation() + (LeftBillboards[i]->GetComponentLocation() + Offset - GetActorLocation()).GetSafeNormal() * TraceLength, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Hit, true);

		if (bHit)
		{
			LeftHitPoints.Add(Hit.ImpactPoint);
		}
		bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), GetActorLocation() + (RightBillboards[i]->GetComponentLocation() + Offset - GetActorLocation()).GetSafeNormal() * TraceLength, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::ForDuration, Hit, true);

		if (bHit)
		{
			RightHitPoints.Add(Hit.ImpactPoint);
		}
	}

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
