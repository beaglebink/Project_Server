#include "Cooking/A_Cookable.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "PhysicsEngine/BodySetup.h"
#include "Cooking/A_Dishes.h"
#include "Components/TextRenderComponent.h"

AA_Cookable::AA_Cookable()
{
	PrimaryActorTick.bCanEverTick = true;

	SlicedMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SlicedMesh"));
	TossTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TossTimelineComponent"));
	DishRatingText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DishRatingText"));

	RootComponent = SlicedMesh;
	DishRatingText->SetupAttachment(RootComponent);

	StaticMesh->SetSimulatePhysics(false);
	StaticMesh->SetNotifyRigidBodyCollision(false);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMesh->SetVisibility(false);

	SlicedMesh->SetCollisionProfileName(TEXT("HighlyReactiveObject"));
	SlicedMesh->bUseComplexAsSimpleCollision = false;
	SlicedMesh->SetSimulatePhysics(true);
	SlicedMesh->SetNotifyRigidBodyCollision(true);
	SlicedMesh->BodyInstance.SetMassOverride(1.0f, true);
	SlicedMesh->SetLinearDamping(0.1f);
	SlicedMesh->SetAngularDamping(0.1f);

	DishRatingText->SetHorizontalAlignment(EHTA_Center);
	DishRatingText->SetRelativeLocation(FVector(0.0f, 0.0f, 20.0f));
	DishRatingText->SetVisibility(false);
}

void AA_Cookable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MeshDynamicMaterial && StaticMesh->GetMaterial(0))
	{
		MeshDynamicMaterial = UMaterialInstanceDynamic::Create(StaticMesh->GetMaterial(0), this);
	}

	if (MeshDynamicMaterial)
	{
		StaticMesh->SetMaterial(0, MeshDynamicMaterial);
	}

	SlicedMesh->ClearAllMeshSections();

	if (StaticMesh->GetStaticMesh())
	{
		StaticMesh->GetStaticMesh()->bAllowCPUAccess = true;
		UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(StaticMesh, 0, SlicedMesh, true);
		if (MeshDynamicMaterial)
		{
			SlicedMesh->SetMaterial(0, MeshDynamicMaterial);
		}
	}

}

void AA_Cookable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Attaching to dish
	if (AttachedDish && bIsAttaching && !bIsHeld)
	{
		OnAttachingPauseCheckTime += GetWorld()->GetDeltaSeconds();
		if (OnAttachingPauseCheckTime > 0.5f && SlicedMesh->GetPhysicsLinearVelocity().Length() < 2.0f && SlicedMesh->GetPhysicsAngularVelocityInDegrees().Length() < 2.0f)
		{
			SlicedMesh->SetSimulatePhysics(false);
			AttachToActor(AttachedDish, FAttachmentTransformRules::KeepWorldTransform);
			bIsAttached = true;
			bIsAttaching = false;
			OnAttachingPauseCheckTime = 0.0f;
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("%2.2f"), ChunkMass));
		}
	}

	//Fall off dish
	if (AttachedDish && !bIsAttaching)
	{
		if (!GetWorldTimerManager().IsTimerActive(AttachedDish->TossTimerHandle) && (FMath::Abs(AttachedDish->GetActorRotation().Roll) >= 80.0f || FMath::Abs(AttachedDish->GetActorRotation().Pitch) >= 80.0f))
		{
			DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			bIsAttached = false;
			SlicedMesh->SetSimulatePhysics(true);
		}
	}
}

void AA_Cookable::BeginPlay()
{
	Super::BeginPlay();

	ChunkMass = SlicedMesh->Bounds.BoxExtent.X * SlicedMesh->Bounds.BoxExtent.Y * SlicedMesh->Bounds.BoxExtent.Z;

	if (TossFloatCurve)
	{
		TossProgressFunction.BindUFunction(this, FName("TossTimelineProgress"));
		TossTimeline->AddInterpFloat(TossFloatCurve, TossProgressFunction);

		TossFinishedFunction.BindUFunction(this, FName("TossTimelineFinished"));
		TossTimeline->SetTimelineFinishedFunc(TossFinishedFunction);

		TossTimeline->SetLooping(false);
	}
}

void AA_Cookable::Destroyed()
{
	Super::Destroyed();

}

void AA_Cookable::HandleCutting_Implementation(UPARAM(ref)FHitResult& Hit, FVector CutPlaneNormal)
{
	UProceduralMeshComponent* OtherHalf = nullptr;

	UKismetProceduralMeshLibrary::SliceProceduralMesh(SlicedMesh, Hit.ImpactPoint, CutPlaneNormal, true, OtherHalf, EProcMeshSliceCapOption::CreateNewSectionForCap, StaticMesh->GetMaterial(0));

	BuildConvexCollision(SlicedMesh);

	if (!OtherHalf)
	{
		return;
	}

	if (AA_Cookable* NewPiece = GetWorld()->SpawnActor<AA_Cookable>(GetClass(), OtherHalf->GetComponentTransform()))
	{
		CopyProceduralMesh(OtherHalf, NewPiece->SlicedMesh);
		OtherHalf->DestroyComponent();
		BuildConvexCollision(NewPiece->SlicedMesh);

		NewPiece->DefaultCookingTime = DefaultCookingTime;
		NewPiece->CookingTime = CookingTime;
		NewPiece->Doneness = Doneness;
		NewPiece->IdealDoneness = IdealDoneness;
		NewPiece->UndercookTolerance = UndercookTolerance;
		NewPiece->OvercookTolerance = OvercookTolerance;
		NewPiece->CookRate = CookRate;
		NewPiece->RecipeImportance = RecipeImportance;
		NewPiece->CriticalRawThreshold = CriticalRawThreshold;
		NewPiece->CriticalBurnThreshold = CriticalBurnThreshold;
		NewPiece->WorstChunkInfluence = WorstChunkInfluence;
		NewPiece->MinimumSignificantChunkSize = MinimumSignificantChunkSize;
		NewPiece->Quality = Quality;

		NewPiece->AddActorWorldOffset(CutPlaneNormal.BackwardVector);

		DistributeMassesByCut(NewPiece);
	}
}

void AA_Cookable::CopyProceduralMesh(UProceduralMeshComponent* Source, UProceduralMeshComponent* Target)
{
	if (!Source || !Target)
	{
		return;
	}

	Target->ClearAllMeshSections();

	const int32 NumSections = Source->GetNumSections();

	for (int32 SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
	{
		FProcMeshSection* Section = Source->GetProcMeshSection(SectionIdx);

		if (!Section)
		{
			continue;
		}

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FLinearColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
		{
			Vertices.Add(Vertex.Position);
			Normals.Add(Vertex.Normal);
			UVs.Add(Vertex.UV0);
			Colors.Add(Vertex.Color);
			Tangents.Add(Vertex.Tangent);
		}


		Triangles.Reserve(Section->ProcIndexBuffer.Num());

		for (uint32 Index : Section->ProcIndexBuffer)
		{
			Triangles.Add(static_cast<int32>(Index));
		}

		Target->CreateMeshSection_LinearColor(SectionIdx, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

		Target->SetMaterial(SectionIdx, Source->GetMaterial(SectionIdx));
	}
}

void AA_Cookable::BuildConvexCollision(UProceduralMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	Mesh->ClearCollisionConvexMeshes();

	const int32 NumSections = Mesh->GetNumSections();

	for (int32 SectionIdx = 0; SectionIdx < NumSections; ++SectionIdx)
	{
		FProcMeshSection* Section = Mesh->GetProcMeshSection(SectionIdx);

		if (!Section)
		{
			continue;
		}

		TArray<FVector> HullVertices;

		for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
		{
			HullVertices.Add(Vertex.Position);
		}

		if (HullVertices.Num() >= 4)
		{
			Mesh->AddCollisionConvexMesh(HullVertices);
		}
	}

	Mesh->bUseComplexAsSimpleCollision = false;

	UBodySetup* BodySetup = Mesh->GetBodySetup();

	if (BodySetup)
	{
		BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	}

	Mesh->RecreatePhysicsState();
}

void AA_Cookable::Toss(float Delta)
{
	if (CookingTime < DefaultCookingTime)
	{
		bWasTossed = true;
	}

	SavedLocalPosition = AttachedDish->GetActorTransform().InverseTransformPosition(GetActorLocation());

	TossStartRotation = GetActorRotation();
	TossTargetRotation = TossStartRotation + FRotator(FMath::RandRange(-90, 90), FMath::RandRange(-90, 90), FMath::RandRange(-90, 90));

	TossOfset = Delta;
	TossTimeline->PlayFromStart();
}

void AA_Cookable::TossTimelineProgress(float Value)
{
	SetActorLocation(FMath::Lerp(AttachedDish->GetActorTransform().TransformPosition(SavedLocalPosition), AttachedDish->GetActorTransform().TransformPosition(SavedLocalPosition) + FVector(0.0f, 0.0f, TossOfset), Value));
	//SetActorRotation(FQuat::Slerp(TossStartRotation.Quaternion(), TossTargetRotation.Quaternion(), Value));
}

void AA_Cookable::TossTimelineFinished()
{
}

void AA_Cookable::IncreaseCookingTime(int32 CookingPeriod)
{
	CookingTime += CookingPeriod;
	Doneness = (static_cast<float>(CookingTime) / static_cast<float>(DefaultCookingTime)) / 2.0f;
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, FString::Printf(TEXT("COOKING %2.2f"), Doneness));
	SetActorScale3D(FMath::Lerp(FVector(1.0f, 1.0f, 1.0f), FVector(0.7f, 0.7f, 0.7f), FMath::Clamp(Doneness, 0.0f, 1.0f)));
	MeshDynamicMaterial->SetScalarParameterValue(FName(TEXT("Cooking")), Doneness);
}

void AA_Cookable::DistributeMassesByCut(AA_Cookable* CutPart)
{
	int32 TriangleCountMainPiece = 0;

	for (int32 SectionIndex = 0; SectionIndex < SlicedMesh->GetNumSections(); ++SectionIndex)
	{
		if (FProcMeshSection* Section = SlicedMesh->GetProcMeshSection(SectionIndex))
		{
			TriangleCountMainPiece += Section->ProcIndexBuffer.Num() / 3;
		}
	}

	int32 TriangleCountCutPart = 0;

	for (int32 SectionIndex = 0; SectionIndex < CutPart->SlicedMesh->GetNumSections(); ++SectionIndex)
	{
		if (FProcMeshSection* Section = CutPart->SlicedMesh->GetProcMeshSection(SectionIndex))
		{
			TriangleCountCutPart += Section->ProcIndexBuffer.Num() / 3;
		}
	}

	const int32 TotalTriangles = TriangleCountMainPiece + TriangleCountCutPart;
	if (TotalTriangles <= 0)
	{
		return;
	}

	float TempMass = ChunkMass;
	ChunkMass = TempMass * TriangleCountMainPiece / TotalTriangles;
	CutPart->ChunkMass = TempMass * TriangleCountCutPart / TotalTriangles;
}

float AA_Cookable::GetChunkQuality()
{
	Quality = 1 - ((Doneness - IdealDoneness) / OvercookTolerance);

	if (Doneness <= IdealDoneness)
	{
		Quality = 1 - ((IdealDoneness - Doneness) / UndercookTolerance);
	}

	Quality = FMath::Clamp(Quality, 0.0f, 1.0f);

	return Quality;
}

EDishRating AA_Cookable::GetDishRating(float DishQuality, const FDishQualityThresholds& Thresholds)
{
	if (DishQuality >= Thresholds.ExcellentThreshold)
	{
		return EDishRating::Excellent;
	}

	if (DishQuality >= Thresholds.GoodThreshold)
	{
		return EDishRating::Good;
	}

	if (DishQuality >= Thresholds.AcceptableThreshold)
	{
		return EDishRating::Acceptable;
	}

	return EDishRating::Poor;
}

void AA_Cookable::ShowFinalDishRating(EDishRating DishRating)
{
	DishRatingText->SetText(StaticEnum<EDishRating>()->GetDisplayNameTextByValue(static_cast<int64>(DishRating)));
	DishRatingText->SetVisibility(true);

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this, DishRating]()
		{
			DishRatingText->SetVisibility(false);
		}, 5.0f, false);
}
