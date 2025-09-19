#include "ChessBoard/A_ChessBoard.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/BoxComponent.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"

AA_ChessBoard::AA_ChessBoard()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}

void AA_ChessBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildField();
}

void AA_ChessBoard::BeginPlay()
{
	Super::BeginPlay();
}

void AA_ChessBoard::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DoCharacterDamage();
}

void AA_ChessBoard::BuildField()
{
	for (FCell& Cell : ChessField)
	{
		if (Cell.MeshComponent)
		{
			Cell.MeshComponent->DestroyComponent();
			Cell.MeshComponent = nullptr;
		}
	}
	ChessField.Empty();

	if (!CellMesh)
	{
		return;
	}

	FVector MeshExtent = CellMesh->GetBounds().BoxExtent * 2;
	FVector CellSize = FVector(MeshExtent.X + 10.0f, MeshExtent.Y + 10.0f, MeshExtent.Z);

	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Col = 0; Col < Columns; ++Col)
		{
			FCell NewCell;
			NewCell.bIsBlack = false;

			// Mesh
			FString CompName = FString::Printf(TEXT("Cell_%d_%d"), Row, Col);
			UStaticMeshComponent* CellComp = NewObject<UStaticMeshComponent>(this, *CompName);

			CellComp->SetStaticMesh(CellMesh);
			CellComp->RegisterComponent();
			CellComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			CellComp->SetRelativeLocation(FVector(Row * CellSize.X, Col * CellSize.Y, 0.0f));
			if ((Row * Columns + Col) & 1)
			{
				NewCell.bIsBlack = true;
				CellComp->SetRelativeRotation(FRotator(0.0f, 0.0f, 180.0f));
			}
			CellComp->SetSimulatePhysics(true);
			CellComp->SetMassOverrideInKg(NAME_None, 10.f);
			CellComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CellComp->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);

			// Constraint
			UPhysicsConstraintComponent* Constraint = NewObject<UPhysicsConstraintComponent>(this);
			Constraint->RegisterComponent();
			Constraint->AttachToComponent(CellComp, FAttachmentTransformRules::KeepRelativeTransform);
			Constraint->SetConstrainedComponents(CellComp, NAME_None, Cast<UPrimitiveComponent>(RootComponent), NAME_None);
			Constraint->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
			Constraint->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
			Constraint->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0.0f);
			Constraint->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0.0f);
			Constraint->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Limited, 10.0f);
			Constraint->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, 10.0f);
			Constraint->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
			Constraint->SetAngularOrientationDrive(true, true);
			Constraint->SetAngularVelocityDriveTwistAndSwing(true, true);
			Constraint->SetAngularDriveParams(1000.0f, 500.0f, 0.0f);

			// Trigger
			FString TriggerName = FString::Printf(TEXT("Trigger_%d_%d"), Row, Col);
			UBoxComponent* Trigger = NewObject<UBoxComponent>(this, *TriggerName);
			Trigger->RegisterComponent();
			Trigger->AttachToComponent(CellComp, FAttachmentTransformRules::KeepRelativeTransform);
			Trigger->SetBoxExtent(FVector(MeshExtent.X * 0.5f, MeshExtent.Y * 0.5f, MeshExtent.Z * 0.5f + 5));
			Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Trigger->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
			Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
			Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			Trigger->OnComponentBeginOverlap.AddDynamic(this, &AA_ChessBoard::OnCellBeginOverlap);
			Trigger->OnComponentEndOverlap.AddDynamic(this, &AA_ChessBoard::OnCellEndOverlap);

			NewCell.MeshComponent = CellComp;
			NewCell.Trigger = Trigger;
			ChessField.Add(NewCell);
		}
	}
}

void AA_ChessBoard::OnCellBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAlsCharacterExample* Ch = Cast<AAlsCharacterExample>(OtherActor))
	{
		FCell* HitCell = ChessField.FindByPredicate([OverlappedComp](const FCell& Cell)
			{
				return Cell.Trigger == OverlappedComp;
			});

		if (HitCell && HitCell->bIsBlack)
		{
			CharactersToCells.FindOrAdd(Ch).Add(HitCell);
		}
	}
}

void AA_ChessBoard::OnCellEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAlsCharacterExample* Ch = Cast<AAlsCharacterExample>(OtherActor))
	{
		FCell* HitCell = ChessField.FindByPredicate([OverlappedComp](const FCell& Cell)
			{
				return Cell.Trigger == OverlappedComp;
			});

		if (HitCell && HitCell->bIsBlack)
		{
			if (TSet<FCell*>* Cells = CharactersToCells.Find(Ch))
			{
				Cells->Remove(HitCell);
				if (Cells->Num() == 0)
				{
					CharactersToCells.Remove(Ch);
				}
			}
		}
	}
}

void AA_ChessBoard::DoCharacterDamage()
{
	for (auto& Pair : CharactersToCells)
	{
		UGameplayStatics::ApplyDamage(Pair.Key, CellDamage, Pair.Key->GetController(), this, nullptr);
	}
}

