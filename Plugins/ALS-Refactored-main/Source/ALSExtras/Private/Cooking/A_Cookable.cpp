#include "Cooking/A_Cookable.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "PhysicsEngine/BodySetup.h"
#include "Cooking/A_Dishes.h"

AA_Cookable::AA_Cookable()
{
	PrimaryActorTick.bCanEverTick = true;

	SlicedMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SlicedMesh"));
	RootComponent = SlicedMesh;

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
}

void AA_Cookable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	SlicedMesh->ClearAllMeshSections();

	if (StaticMesh->GetStaticMesh())
	{
		StaticMesh->GetStaticMesh()->bAllowCPUAccess = true;
		UKismetProceduralMeshLibrary::CopyProceduralMeshFromStaticMeshComponent(StaticMesh, 0, SlicedMesh, true);
	}
}

void AA_Cookable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
		}
	}

	if (AttachedDish && !bIsAttaching)
	{
		if (abs(AttachedDish->GetActorRotation().Roll) >= 80.0f || abs(AttachedDish->GetActorRotation().Pitch) >= 80.0f)
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

	AA_Cookable* NewPiece = GetWorld()->SpawnActor<AA_Cookable>(GetClass(), OtherHalf->GetComponentTransform());

	if (!NewPiece)
	{
		return;
	}

	CopyProceduralMesh(OtherHalf, NewPiece->SlicedMesh);
	BuildConvexCollision(NewPiece->SlicedMesh);

	OtherHalf->DestroyComponent();
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