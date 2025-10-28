#include "EnvironmentalObstacles/A_WireObstacle.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"

AA_WireObstacle::AA_WireObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	RemoveWireTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineComponent"));

	BoxComponent->SetupAttachment(RootComponent);
	AudioComponent->SetupAttachment(RootComponent);

	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxComponent->SetCollisionObjectType(ECC_WorldDynamic);
	BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	BoxComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Ignore);
	AudioComponent->bAutoActivate = false;
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
	float LowestPointZ = INT_MAX;

	LeftHitPoints.Empty();
	RightHitPoints.Empty();

	for (size_t i = 0; i < LeftAnchors.Num(); ++i)
	{
		FVector Offset = FVector(FMath::RandRange(-TraceSpread, TraceSpread), 0.0f, 0.0f);
		FHitResult Hit;
		TArray<AActor*> ActorsToIgnore;
		ActorsToIgnore.Add(this);

		bool bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), LeftAnchors[i] + Offset, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

		if (bHit)
		{
			LeftHitPoints.Add(Hit.ImpactPoint);
			LowestPointZ = FMath::Min(LowestPointZ, Hit.ImpactPoint.Z);
		}

		bHit = UKismetSystemLibrary::LineTraceSingle(GetWorld(), GetActorLocation(), RightAnchors[i] + Offset, ETraceTypeQuery::TraceTypeQuery1, false, ActorsToIgnore, EDrawDebugTrace::None, Hit, true);

		if (bHit)
		{
			RightHitPoints.Add(Hit.ImpactPoint);
			LowestPointZ = FMath::Min(LowestPointZ, Hit.ImpactPoint.Z);
		}
	}

	//Find boxcollision size
	if (!LeftHitPoints.IsEmpty() && !RightHitPoints.IsEmpty())
	{
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
		BoxComponent->SetBoxExtent(FVector(2.0f, BoxWidth / 2.0f, BoxHeight / 2.0f));
		BoxComponent->SetWorldLocation(BoxLocation);

		//Clear splines
		for (USplineComponent* Spline : WireSplines)
		{
			if (Spline)
			{
				Spline->DestroyComponent();
			}
		}
		WireSplines.Empty();

		//Build wires splines
		ShuffleArray(LeftHitPoints);
		ShuffleArray(RightHitPoints);
		bool bShouldChangeSide = false;

		for (size_t i = 0; i < FMath::Min(LeftHitPoints.Num(), RightHitPoints.Num()); ++i)
		{
			FVector Start = LeftHitPoints[i];
			FVector End = RightHitPoints[i];
			if (bShouldChangeSide)
			{
				Start = RightHitPoints[i];
				End = LeftHitPoints[i];
			}
			bShouldChangeSide = !bShouldChangeSide;

			FVector MidPoint = (Start + End) * 0.5f;

			USplineComponent* WireSpline = NewObject<USplineComponent>(this);
			WireSpline->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
			WireSpline->SetWorldLocation(Start);
			WireSpline->SetLocationAtSplinePoint(1, MidPoint, ESplineCoordinateSpace::World);
			WireSpline->AddSplinePoint(End, ESplineCoordinateSpace::World);

			float RandTimeOnSpline = FMath::FRandRange(20.0f, 80.0f) / 100.0f;
			MidPoint = WireSpline->GetLocationAtTime(RandTimeOnSpline, ESplineCoordinateSpace::World);
			MidPoint.Z -= FMath::RandRange(15.0f, 60.0f);
			MidPoint.Z = FMath::Max(MidPoint.Z, LowestPointZ);
			WireSpline->SetLocationAtSplinePoint(1, MidPoint, ESplineCoordinateSpace::World);

			WireSpline->SetSplinePointType(0, ESplinePointType::Curve);
			WireSpline->SetSplinePointType(1, ESplinePointType::Curve);
			WireSpline->SetSplinePointType(2, ESplinePointType::Curve);
			WireSpline->RegisterComponent();

			WireSplines.Add(WireSpline);
		}

		//Spawn nodes
		for (UPrimitiveComponent* MeshComp : Nodes)
		{
			if (MeshComp)
			{
				MeshComp->DestroyComponent();
			}
		}
		Nodes.Empty();

		if (NodeMesh)
		{
			ShuffleArray(WireSplines);
			for (size_t i = 0; i < NodesQuantity; ++i)
			{
				FString NodeName = "NodeMeshComp_" + LexToString(i + 1);
				UStaticMeshComponent* StaticMeshComp = NewObject<UStaticMeshComponent>(this, FName(NodeName));
				StaticMeshComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
				StaticMeshComp->SetStaticMesh(NodeMesh);
				StaticMeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
				StaticMeshComp->SetCollisionObjectType(ECC_WorldDynamic);
				StaticMeshComp->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
				StaticMeshComp->SetWorldLocation(WireSplines[i]->GetLocationAtSplinePoint(1, ESplineCoordinateSpace::World));
				StaticMeshComp->RegisterComponent();

				Nodes.Add(StaticMeshComp);
			}
		}
	}
}

void AA_WireObstacle::BeginPlay()
{
	Super::BeginPlay();

	//Build wires
	if (WireFX)
	{
		for (USplineComponent* WireSpline : WireSplines)
		{
			UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(this);
			NiagaraComp->AttachToComponent(WireSpline, FAttachmentTransformRules::KeepRelativeTransform);
			NiagaraComp->SetAsset(WireFX);
			NiagaraComp->SetVariableObject(FName(TEXT("User.SplineObject")), WireSpline);
			NiagaraComp->RegisterComponent();
			NiagaraComp->Activate();

			WireFXArray.Add(NiagaraComp);
		}
	}

	//Timeline
	if (RemoveWireFloatCurve)
	{
		RemoveWireProgressFunction.BindUFunction(this, FName("RemoveWireTimelineProgress"));
		RemoveWireTimeline->AddInterpFloat(RemoveWireFloatCurve, RemoveWireProgressFunction);

		RemoveWireFinishedFunction.BindUFunction(this, FName("RemoveWireTimelineFinished"));
		RemoveWireTimeline->SetTimelineFinishedFunc(RemoveWireFinishedFunction);

		RemoveWireTimeline->SetLooping(false);
	}
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

void AA_WireObstacle::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	for (UPrimitiveComponent* Comp : Nodes)
	{
		if (Hit.GetComponent() == Comp)
		{
			Comp->DestroyComponent();
			Nodes.Remove(Comp);
			Comp = nullptr;
			break;
		}
	}
	if (Nodes.IsEmpty())
	{
		AudioComponent->Play();
		RemoveWireTimeline->PlayFromStart();
	}
}

void AA_WireObstacle::RemoveWireTimelineProgress(float Value)
{
	for (UNiagaraComponent* NiagaraComp : WireFXArray)
	{
		NiagaraComp->SetVariableFloat(FName(TEXT("CurrentNormalizedIndex")), Value);
	}
}

void AA_WireObstacle::RemoveWireTimelineFinished()
{
	AudioComponent->Stop();
	Destroy();
}

