#include "ChessBoard/A_ChessBoard.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "AlsCharacterExample.h"
#include "Kismet/GameplayStatics.h"

AA_ChessBoard::AA_ChessBoard()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RotateTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("RotateTimeline"));
}

#if WITH_EDITOR
void AA_ChessBoard::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();

		if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_ChessBoard, MinTime) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(AA_ChessBoard, MaxTime))
		{
			MaxTime = FMath::Max(MaxTime, MinTime);
		}
	}
}
#endif

void AA_ChessBoard::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BuildField();
}

void AA_ChessBoard::BeginPlay()
{
	Super::BeginPlay();

	if (RotateFloatCurve)
	{
		RotateProgressFunction.BindUFunction(this, FName("RotateTimelineProgress"));
		RotateTimeline->AddInterpFloat(RotateFloatCurve, RotateProgressFunction);

		RotateFinishedFunction.BindUFunction(this, FName("RotateTimelineFinished"));
		RotateTimeline->SetTimelineFinishedFunc(RotateFinishedFunction);

		RotateTimeline->SetLooping(false);
	}

	FTimerHandle FirstTimerHandle;
	GetWorldTimerManager().SetTimer(FirstTimerHandle, this, &AA_ChessBoard::ScheduleNextTimer, FMath::FRandRange(MinTime, MaxTime), false);
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
		if (Cell.ConstraintComponent)
		{
			Cell.ConstraintComponent->DestroyComponent();
			Cell.ConstraintComponent = nullptr;
		}
		if (Cell.TriggerComponent)
		{
			Cell.TriggerComponent->DestroyComponent();
			Cell.TriggerComponent = nullptr;
		}
		if (Cell.AudioComponent)
		{
			Cell.AudioComponent->DestroyComponent();
			Cell.AudioComponent = nullptr;
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

			// Mesh
			FString CompName = FString::Printf(TEXT("Cell_%d_%d"), Row, Col);
			UStaticMeshComponent* CellComp = NewObject<UStaticMeshComponent>(this, *CompName);

			CellComp->SetStaticMesh(CellMesh);
			CellComp->RegisterComponent();
			CellComp->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			CellComp->SetRelativeLocation(FVector(Row * CellSize.X, Col * CellSize.Y, 0.0f));
			CellComp->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
			if ((Row + Col) & 1)
			{
				CellComp->SetRelativeRotation(FRotator(0.0f, 0.0f, 180.0f));
			}
			CellComp->SetSimulatePhysics(true);
			CellComp->SetMassOverrideInKg(NAME_None, 10.0f);
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
			Trigger->SetBoxExtent(FVector(MeshExtent.X * 0.45f, MeshExtent.Y * 0.45f, 5.0f));
			Trigger->SetRelativeLocation(FVector(0.0f, 0.0f, -MeshExtent.Z * 0.5f));
			Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Trigger->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
			Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
			Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
			Trigger->OnComponentBeginOverlap.AddDynamic(this, &AA_ChessBoard::OnCellBeginOverlap);
			Trigger->OnComponentEndOverlap.AddDynamic(this, &AA_ChessBoard::OnCellEndOverlap);

			// Audio
			FString AudioName = FString::Printf(TEXT("Audio_%d_%d"), Row, Col);
			UAudioComponent* AudioComp = NewObject<UAudioComponent>(this, *AudioName);
			AudioComp->RegisterComponent();
			AudioComp->AttachToComponent(CellComp, FAttachmentTransformRules::KeepRelativeTransform);
			AudioComp->bAutoActivate = false;
			AudioComp->SetSound(RotateSound);

			// Cell
			NewCell.MeshComponent = CellComp;
			NewCell.ConstraintComponent = Constraint;
			NewCell.TriggerComponent = Trigger;
			NewCell.AudioComponent = AudioComp;
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
				return Cell.TriggerComponent == OverlappedComp;
			});

		if (HitCell)
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
				return Cell.TriggerComponent == OverlappedComp;
			});

		if (HitCell)
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

void AA_ChessBoard::ScheduleNextTimer()
{
	if (2 * RotateTimeline->GetTimelineLength() > MinTime)
	{
		return;
	}

	RotateCells();

	float Delay = FMath::FRandRange(MinTime, MaxTime);

	FTimerHandle RandomTimerHandle;
	GetWorldTimerManager().SetTimer(RandomTimerHandle, this, &AA_ChessBoard::ScheduleNextTimer, Delay, false);
}

void AA_ChessBoard::RotateCells()
{

	for (size_t Index = 0; Index < ChessField.Num(); ++Index)
	{
		if (abs(ChessField[Index].MeshComponent->GetComponentRotation().Pitch) > 90.0f || abs(ChessField[Index].MeshComponent->GetComponentRotation().Roll) > 90.0f)
		{
			TargetRotation = CountTargetRotationInDependsOnPrevRotation(ChessField[Index].MeshComponent->GetComponentRotation());

			CellsToRotate.Add(Index);
			ChessField[Index].AudioComponent->Play();
			InitialRotations.Add(Index, ChessField[Index].MeshComponent->GetComponentRotation());
		}
	}
	RotateTimeline->PlayFromStart();

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			int32 NumToRotate = static_cast<int32>(FMath::FRandRange(ChessField.Num() * 0.3f, ChessField.Num() * 0.4f));

			for (size_t i = 0; i < ChessField.Num(); ++i)
			{
				CellsToRotate.Add(i);
			}

			for (int32 i = CellsToRotate.Num() - 1; i > 0; --i)
			{
				int32 j = FMath::RandRange(0, i);
				CellsToRotate.Swap(i, j);
			}
			CellsToRotate.SetNum(NumToRotate);

			for (int32 Index : CellsToRotate)
			{
				TargetRotation = CountTargetRotationInDependsOnPrevRotation(ChessField[Index].MeshComponent->GetComponentRotation());

				ChessField[Index].AudioComponent->Play();
				InitialRotations.Add(Index, ChessField[Index].MeshComponent->GetComponentRotation());
			}

			RotateTimeline->PlayFromStart();
		}, RotateTimeline->GetTimelineLength(), false);
}

FQuat AA_ChessBoard::CountTargetRotationInDependsOnPrevRotation(const FRotator& CurrentRot)
{
	FRotator TargetRot = CurrentRot;

	bool bIsWhiteField = (FMath::Abs(TargetRot.Pitch) < 90.0f && FMath::Abs(TargetRot.Roll) < 90.0f) || (FMath::Abs(TargetRot.Pitch) > 90.0f && FMath::Abs(TargetRot.Roll) > 90.0f);

	bool bUsePitchAsLead = false;

	if (bIsWhiteField)
	{
		if ((FMath::Abs(TargetRot.Pitch) < 90.0f && FMath::Abs(TargetRot.Roll) < 90.0f))
		{
			bUsePitchAsLead = FMath::Abs(TargetRot.Pitch) >= FMath::Abs(TargetRot.Roll);
		}
		else
		{
			bUsePitchAsLead = FMath::Abs(TargetRot.Pitch) <= FMath::Abs(TargetRot.Roll);
		}
	}
	else
	{
		float DeltaPitch = FMath::Abs((FMath::Abs(TargetRot.Pitch) > 90.0f ? 180.0f : 0.0f) - FMath::Abs(TargetRot.Pitch));
		float DeltaRoll = FMath::Abs((FMath::Abs(TargetRot.Roll) > 90.0f ? 180.0f : 0.0f) - FMath::Abs(TargetRot.Roll));
		bUsePitchAsLead = DeltaPitch >= DeltaRoll;
	}

	if (bUsePitchAsLead)
	{
		float DeltaPitch = (FMath::Abs(TargetRot.Pitch) > 90.0f ? 0.0f : 180.0f) - TargetRot.Pitch;
		TargetRot.Pitch += DeltaPitch;

		float DeltaRoll = (FMath::Abs(TargetRot.Roll) > 90.0f ? 180.0f : 0.0f) - TargetRot.Roll;
		TargetRot.Roll += DeltaRoll;
	}
	else
	{
		float DeltaPitch = (FMath::Abs(TargetRot.Pitch) > 90.0f ? 180.0f : 0.0f) - TargetRot.Pitch;
		TargetRot.Pitch += DeltaPitch;

		float DeltaRoll = (FMath::Abs(TargetRot.Roll) > 90.0f ? 0.0f : 180.0f) - TargetRot.Roll;
		TargetRot.Roll += DeltaRoll;
	}

	TargetRot.Normalize();
	return TargetRot.Quaternion();
}

void AA_ChessBoard::RotateTimelineProgress(float Value)
{
	for (int32 CellIndex : CellsToRotate)
	{
		if (ChessField.IsValidIndex(CellIndex))
		{
			ChessField[CellIndex].MeshComponent->SetSimulatePhysics(false);
			FQuat NewOrientation = FQuat::Slerp(InitialRotations[CellIndex].Quaternion(), TargetRotation, Value);
			ChessField[CellIndex].MeshComponent->SetWorldRotation(NewOrientation.Rotator());
		}
	}
}

void AA_ChessBoard::RotateTimelineFinished()
{
	for (int32 CellIndex : CellsToRotate)
	{
		if (ChessField.IsValidIndex(CellIndex))
		{
			ChessField[CellIndex].MeshComponent->SetSimulatePhysics(true);
			ChessField[CellIndex].ConstraintComponent->SetConstrainedComponents(ChessField[CellIndex].MeshComponent, NAME_None, Cast<UPrimitiveComponent>(RootComponent), NAME_None);
			ChessField[CellIndex].AudioComponent->Stop();
		}
	}

	CellsToRotate.Empty();
	InitialRotations.Empty();
}

