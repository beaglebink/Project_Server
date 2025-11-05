#include "EnvironmentalObstacles/A_BlockObstacle.h"
#include "EnvironmentalObstacles/A_BlocksWallObstacle.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"

AA_BlockObstacle::AA_BlockObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	BlockMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlockMeshComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	BlockDestroyFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BlockDestroyFXComponent"));
	DestroyTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DestroyTimelineComponent"));
	MoveOnDirectionBlockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveOnDirectionTimelineComponent"));
	FrontBackBlockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("FrontBackTimelineComponent"));

	BlockMeshComponent->SetupAttachment(RootComponent);
	AudioComponent->SetupAttachment(RootComponent);
	BlockDestroyFX->SetupAttachment(BlockMeshComponent);

	AudioComponent->bAutoActivate = false;
	BlockDestroyFX->bAutoActivate = false;
}

void AA_BlockObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BlockExtent = BlockMeshComponent->GetStaticMesh()->GetExtendedBounds().BoxExtent * 2.0f;

	if (BlockMaterialAsset && !IsValid(BlockMaterialInstance))
	{
		BlockMaterialInstance = UMaterialInstanceDynamic::Create(BlockMaterialAsset, BlockMeshComponent);
		BlockMaterialInstance->SetScalarParameterValue(FName("IsCritical"), 0);
		BlockMeshComponent->SetMaterial(0, BlockMaterialInstance);
	}
}

void AA_BlockObstacle::BeginPlay()
{
	Super::BeginPlay();

	GridOfBlocks = Cast<AA_BlocksWallObstacle>(GetOwner());

	//Destroy timeline
	if (DestroyFloatCurve)
	{
		DestroyProgressFunction.BindUFunction(this, FName("DestroyTimelineProgress"));
		DestroyTimeline->AddInterpFloat(DestroyFloatCurve, DestroyProgressFunction);

		DestroyFinishedFunction.BindUFunction(this, FName("DestroyTimelineFinished"));
		DestroyTimeline->SetTimelineFinishedFunc(DestroyFinishedFunction);

		DestroyTimeline->SetLooping(false);
	}

	//MoveOnDirection timeline 
	if (MoveOnDirectionBlockFloatCurve)
	{
		MoveOnDirectionBlockProgressFunction.BindUFunction(this, FName("MoveOnDirectionBlockTimelineProgress"));
		MoveOnDirectionBlockTimeline->AddInterpFloat(MoveOnDirectionBlockFloatCurve, MoveOnDirectionBlockProgressFunction);

		MoveOnDirectionBlockFinishedFunction.BindUFunction(this, FName("MoveOnDirectionBlockTimelineFinished"));
		MoveOnDirectionBlockTimeline->SetTimelineFinishedFunc(MoveOnDirectionBlockFinishedFunction);

		MoveOnDirectionBlockTimeline->SetLooping(false);
	}

	//FrontBack timeline 
	if (FrontBackBlockFloatCurve)
	{
		FrontBackBlockProgressFunction.BindUFunction(this, FName("FrontBackBlockTimelineProgress"));
		FrontBackBlockTimeline->AddInterpFloat(FrontBackBlockFloatCurve, FrontBackBlockProgressFunction);

		FrontBackBlockFinishedFunction.BindUFunction(this, FName("FrontBackBlockTimelineFinished"));
		FrontBackBlockTimeline->SetTimelineFinishedFunc(FrontBackBlockFinishedFunction);

		FrontBackBlockTimeline->SetLooping(false);
	}
}

void AA_BlockObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsFalling)
	{
		float PrevSpeed = FallSpeed;
		FallSpeed += 98.0f * DeltaTime;


		FVector Next = GetActorLocation() - FVector(0.0f, 0.0f, FallSpeed * DeltaTime);

		if (LowerBlock)
		{
			float Distance = FVector::Distance(Next, LowerBlock->GetActorLocation());
			if (Distance <= BlockExtent.Z)
			{
				FallSpeed = LowerBlock->FallSpeed;
				Next.Z = LowerBlock->GetActorLocation().Z + BlockExtent.Z;
				if (!LowerBlock->bIsFalling)
				{
					bIsFalling = false;
					InitialLocation = TargetLocation;
				}
			}
		}
		else if (Next.Z <= TargetLocation.Z)
		{
			Next.Z = TargetLocation.Z;
			FallSpeed = 0.0f;
			bIsFalling = false;
			InitialLocation = TargetLocation;
		}

		SetActorLocation(Next);
		if (PrevSpeed - FallSpeed > 0.0f)
		{
			AudioComponent->SetSound(BlockFallSound);
			AudioComponent->Play();
		}
	}
}

void AA_BlockObstacle::SetBlockState(EBlockState NewState)
{
	switch (NewState)
	{
	case EBlockState::Neutral:
	{
		BlockState = EBlockState::Neutral;
		SetBlockMaterial(false);
		break;
	}
	case EBlockState::Critical:
	{
		BlockState = EBlockState::Critical;
		SetBlockMaterial(true);
		break;
	}
	default:
		break;
	}
}

void AA_BlockObstacle::SetBlockMaterial(bool bIsCritical)
{
	BlockMaterialInstance->SetScalarParameterValue(FName("IsCritical"), static_cast<int32>(bIsCritical));
}

void AA_BlockObstacle::HandleShotForMaterial()
{
	OnShotMaterial(true);
	FTimerHandle ResetShotHandle;
	GetWorldTimerManager().SetTimer(ResetShotHandle, [this]()
		{
			OnShotMaterial(false);
		}, 0.2f, false);
}

void AA_BlockObstacle::OnShotMaterial(bool bIsShot)
{
	BlockMaterialInstance->SetScalarParameterValue(FName("EmissiveIntensity"), 1 + static_cast<int32>(bIsShot) * 10);
}

void AA_BlockObstacle::StartBlockDestroy()
{
	UMaterialInstanceDynamic* DynMat = nullptr;
	DynMat = UMaterialInstanceDynamic::Create(BlockMaterialAsset, BlockDestroyFX);
	DynMat->SetScalarParameterValue(FName(TEXT("IsNiagaraMaterial")), 1);
	DynMat->SetScalarParameterValue(FName(TEXT("IsCritical")), static_cast<float>(BlockState));
	BlockDestroyFX->SetVariableMaterial(FName(TEXT("User.ParticlesMaterial")), DynMat);
	BlockDestroyFX->Activate(true);

	AudioComponent->SetSound(BlockDestroySound);
	AudioComponent->Play();
	DestroyTimeline->PlayFromStart();
}

void AA_BlockObstacle::MoveOnDirectionBlock(EDirection Direction)
{
	FVector Offset;
	switch (Direction)
	{
	case EDirection::Up:
	{
		Offset = GetActorUpVector() * BlockExtent.Z;
		break;
	}
	case EDirection::Right:
	{
		Offset = -GetActorRightVector() * BlockExtent.Y;
		break;
	}
	case EDirection::Down:
	{
		Offset = -GetActorUpVector() * BlockExtent.Z;
		break;
	}
	case EDirection::Left:
	{
		Offset = GetActorRightVector() * BlockExtent.Y;
		break;
	}
	default:
		break;
	}
	TargetLocation = InitialLocation + Offset;
	if (BlockState == EBlockState::Critical)
	{
		AudioComponent->SetSound(BlockMoveOnDirectionSound);
		AudioComponent->Play();
		//Pull block
		FrontBackBlockTimeline->PlayFromStart();

		//Push block back
		FTimerHandle PushHandle;
		GetWorldTimerManager().SetTimer(PushHandle, [this]()
			{
				FrontBackBlockTimeline->PlayFromStart();
			}, FrontBackBlockTimeline->GetTimelineLength() + MoveOnDirectionBlockTimeline->GetTimelineLength(), false);
	}
	else
	{
		FTimerHandle MoveHandle;
		GetWorldTimerManager().SetTimer(MoveHandle, [this]()
			{
				MoveOnDirectionBlockTimeline->PlayFromStart();
			}, FrontBackBlockTimeline->GetTimelineLength(), false);
	}
}

void AA_BlockObstacle::StartBlockFall(AA_BlockObstacle* Block)
{
	LowerBlock = Block;
	TargetLocation -= BlockExtent.Z * GetActorUpVector();
	bIsFalling = true;
}

void AA_BlockObstacle::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	if (BlockState == EBlockState::Critical && !GridOfBlocks->bIsProcessingSwaps && !bIsOnDestroying)
	{
		bIsOnDestroying = true;
		GridOfBlocks->NotifyBlockDestroyed();
		GridOfBlocks->NotifyPlayerShot(true);
		StartBlockDestroy();
	}
	else
	{
		HandleShotForMaterial();
		GridOfBlocks->NotifyPlayerShot(false);
	}
}

void AA_BlockObstacle::DestroyTimelineProgress(float Value)
{
	BlockMaterialInstance->SetScalarParameterValue(FName(TEXT("Opacity")), Value);
}
void AA_BlockObstacle::DestroyTimelineFinished()
{
	GridOfBlocks->UpperBlocksFall(BlockIndex);

	AudioComponent->Stop();
	Destroy();
}

void AA_BlockObstacle::MoveOnDirectionBlockTimelineProgress(float Value)
{
	SetActorLocation(FMath::Lerp(InitialLocation, TargetLocation, Value));
}

void AA_BlockObstacle::MoveOnDirectionBlockTimelineFinished()
{
	InitialLocation = TargetLocation;
	if (BlockState == EBlockState::Neutral)
	{
		GridOfBlocks->CheckAndHandleCompletedSwaps();
	}
}

void AA_BlockObstacle::FrontBackBlockTimelineProgress(float Value)
{
	SetActorLocation(FMath::Lerp(InitialLocation, InitialLocation + GetActorForwardVector() * BlockExtent.X * (bIsPulling * 2 - 1), Value));
}

void AA_BlockObstacle::FrontBackBlockTimelineFinished()
{
	InitialLocation = GetActorLocation();

	if (bIsPulling)
	{
		bIsPulling = false;
		TargetLocation = TargetLocation + GetActorForwardVector() * BlockExtent.X;
		MoveOnDirectionBlockTimeline->PlayFromStart();
	}
	else
	{
		bIsPulling = true;
		TargetLocation = InitialLocation;
		GridOfBlocks->CheckAndHandleCompletedSwaps();
		AudioComponent->Stop();
	}
}

