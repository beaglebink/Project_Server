#include "EnvironmentalObstacles/A_BlocksWallObstacle.h"
#include "Engine/StaticMesh.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AA_BlocksWallObstacle::AA_BlocksWallObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	DestroyTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DestroyTimelineComponent"));
	MoveBlockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("MoveTimelineComponent"));

	AudioComponent->SetupAttachment(RootComponent);

	AudioComponent->bAutoActivate = false;
}

#if WITH_EDITOR
void AA_BlocksWallObstacle::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_BlocksWallObstacle, bUseTimeInterval))
	{
		if (bUseTimeInterval)
		{
			bUseNumberOfBlocksDestroyed = false;
			bUsePlayerAccuracy = false;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_BlocksWallObstacle, bUseNumberOfBlocksDestroyed))
	{
		if (bUseNumberOfBlocksDestroyed)
		{
			bUseTimeInterval = false;
			bUsePlayerAccuracy = false;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_BlocksWallObstacle, bUsePlayerAccuracy))
	{
		if (bUsePlayerAccuracy)
		{
			bUseTimeInterval = false;
			bUseNumberOfBlocksDestroyed = false;
		}
	}
}
#endif

void AA_BlocksWallObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Clear old blocks
	for (FBlockRow& Row : WallBlocks)
	{
		for (FWallBlock& Block : Row.Row)
		{
			if (Block.BlockMeshComponent)
			{
				Block.BlockMeshComponent->DestroyComponent();
			}
		}
	}
	WallBlocks.Empty();

	if (!BlockMeshAsset)
	{
		return;
	}

	const FVector BlockSize = BlockMeshAsset->GetBounds().BoxExtent * 2.0f;

	for (int32 Y = 0; Y < WallDimensions.Y; ++Y)
	{
		FBlockRow NewRow;

		for (int32 X = 0; X < WallDimensions.X; ++X)
		{
			FWallBlock NewBlock;

			const FName CompName = *FString::Printf(TEXT("Block_%d_%d"), X, Y);
			UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this, CompName);
			MeshComp->SetupAttachment(RootComponent);
			MeshComp->RegisterComponent();
			MeshComp->SetStaticMesh(BlockMeshAsset);

			const FVector Offset(0.f, -X * BlockSize.Y, -Y * BlockSize.Z);
			MeshComp->SetRelativeLocation(Offset);

			UMaterialInstanceDynamic* DynMat = nullptr;
			if (MeshComp && BlockMaterialAsset)
			{
				DynMat = UMaterialInstanceDynamic::Create(BlockMaterialAsset, MeshComp);
				DynMat->SetScalarParameterValue(FName("IsCritical"), 0);
				MeshComp->SetMaterial(0, DynMat);
			}

			NewBlock.BlockMeshComponent = MeshComp;
			NewBlock.InitialLocation = GetActorLocation() + Offset;
			NewBlock.TargetLocation = NewBlock.InitialLocation;
			NewBlock.BlockMaterialInstance = DynMat;
			NewBlock.GridPosition = FIntPoint(X, Y);
			NewBlock.BlockState = EBlockState::Neutral;

			NewRow.Row.Add(NewBlock);
		}
		WallBlocks.Add(NewRow);
	}

	//Set Critical blocks
	const int32 TotalBlocks = WallDimensions.X * WallDimensions.Y;
	const int32 CriticalCount = FMath::RoundToInt(TotalBlocks * CriticalBlocksTreshold / 100.0f);

	TArray<int32> Indices;
	Indices.Reserve(TotalBlocks);

	for (int32 i = 0; i < TotalBlocks; ++i)
	{
		Indices.Add(i);
	}

	for (int32 i = 0; i < TotalBlocks; ++i)
	{
		int32 SwapIdx = FMath::RandRange(i, TotalBlocks - 1);
		Indices.Swap(i, SwapIdx);
	}

	for (int32 i = 0; i < CriticalCount; ++i)
	{
		int32 RandomIndex = Indices[i];

		int32 Y = RandomIndex / WallDimensions.X;
		int32 X = RandomIndex % WallDimensions.X;

		if (WallBlocks.IsValidIndex(Y) && WallBlocks[Y].Row.IsValidIndex(X))
		{
			FWallBlock& Block = WallBlocks[Y].Row[X];
			SetBlockState(Block.GridPosition, EBlockState::Critical);
		}
	}
}

void AA_BlocksWallObstacle::BeginPlay()
{
	Super::BeginPlay();

	//Destroy timeline
	if (DestroyFloatCurve)
	{
		DestroyProgressFunction.BindUFunction(this, FName("DestroyTimelineProgress"));
		DestroyTimeline->AddInterpFloat(DestroyFloatCurve, DestroyProgressFunction);

		DestroyFinishedFunction.BindUFunction(this, FName("DestroyTimelineFinished"));
		DestroyTimeline->SetTimelineFinishedFunc(DestroyFinishedFunction);

		DestroyTimeline->SetLooping(false);
	}

	//Move timeline 
	if (MoveBlockFloatCurve)
	{
		MoveBlockProgressFunction.BindUFunction(this, FName("MoveBlockTimelineProgress"));
		MoveBlockTimeline->AddInterpFloat(MoveBlockFloatCurve, MoveBlockProgressFunction);

		MoveBlockFinishedFunction.BindUFunction(this, FName("MoveBlockTimelineFinished"));
		MoveBlockTimeline->SetTimelineFinishedFunc(MoveBlockFinishedFunction);

		MoveBlockTimeline->SetLooping(false);
	}
}

void AA_BlocksWallObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_BlocksWallObstacle::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	if (bBlockCoolDown)
	{
		return;
	}
	bBlockCoolDown = true;

	FTimerHandle BlockCoolDownHandle;
	GetWorldTimerManager().SetTimer(BlockCoolDownHandle, [this]()
		{
			bBlockCoolDown = false;
		}, 0.5f, false);

	for (FBlockRow& Rows : WallBlocks)
	{
		for (FWallBlock& Block : Rows.Row)
		{
			if (Block.BlockMeshComponent == Hit.GetComponent())
			{
				if (Block.BlockState == EBlockState::Critical)
				{
					CurrentBlock = &Block;
					SetBlockState(Block.GridPosition, EBlockState::Destroyed);
				}
				else if (Block.BlockState == EBlockState::Neutral)
				{
					SetBlockState(Block.GridPosition, EBlockState::Shot);
				}
				return;
			}
		}
	}
}

void AA_BlocksWallObstacle::SetBlockState(FIntPoint GridPosition, EBlockState NewState)
{
	switch (NewState)
	{
	case EBlockState::Neutral:
	{
		WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Neutral;
		SetBlockMaterial(GridPosition, false);
		break;
	}
	case EBlockState::Critical:
	{
		WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Critical;
		SetBlockMaterial(GridPosition, true);
		break;
	}
	case EBlockState::Destroyed:
	{
		WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Destroyed;
		StartBlockDestroy(GridPosition);
		break;
	}
	case EBlockState::Moving:
	{
		WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Moving;
		MoveBlock(GridPosition);
		break;
	}
	case EBlockState::Falling:
	{
		WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Falling;
		StartBlockFall(GridPosition);
		break;
	}
	case EBlockState::Shot:
	{
		WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Shot;
		OnShotMaterial(GridPosition, true);
		FTimerHandle ResetShotHandle;
		GetWorldTimerManager().SetTimer(ResetShotHandle, [this, GridPosition]()
			{
				OnShotMaterial(GridPosition, false);
				WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockState = EBlockState::Neutral;
			}, 0.2f, false);
		break;
	}
	default:
		break;
	}
}

void AA_BlocksWallObstacle::SetBlockMaterial(FIntPoint GridPosition, bool bIsCritical)
{
	WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockMaterialInstance->SetScalarParameterValue(FName("IsCritical"), static_cast<int32>(bIsCritical));
}

void AA_BlocksWallObstacle::OnShotMaterial(FIntPoint GridPosition, bool bIsShot)
{
	WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockMaterialInstance->SetScalarParameterValue(FName("EmissiveIntensity"), 1 + static_cast<int32>(bIsShot) * 10);
}

void AA_BlocksWallObstacle::StartBlockDestroy(FIntPoint GridPosition)
{
	if (BlockDestroyFX)
	{
		UNiagaraComponent* NiagaraComp = NewObject<UNiagaraComponent>(this);
		NiagaraComp->SetAsset(BlockDestroyFX);
		NiagaraComp->AttachToComponent(WallBlocks[GridPosition.Y].Row[GridPosition.X].BlockMeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
		NiagaraComp->RegisterComponent();
		UMaterialInstanceDynamic* DynMat = nullptr;
		if (BlockMaterialAsset)
		{
			DynMat = UMaterialInstanceDynamic::Create(BlockMaterialAsset, NiagaraComp);
			DynMat->SetScalarParameterValue(FName(TEXT("IsNiagaraMaterial")), 1);
			DynMat->SetScalarParameterValue(FName(TEXT("IsCritical")), 1);
			NiagaraComp->SetVariableMaterial(FName(TEXT("User.ParticlesMaterial")), DynMat);
		}
		NiagaraComp->Activate();
	}
	DestroyTimeline->PlayFromStart();
	AudioComponent->SetSound(BlockDestroySound);
	AudioComponent->Play();
}

void AA_BlocksWallObstacle::MoveBlock(FIntPoint GridPosition)
{
}

void AA_BlocksWallObstacle::StartBlockFall(FIntPoint GridPosition)
{
}

void AA_BlocksWallObstacle::DestroyTimelineProgress(float Value)
{
	CurrentBlock->BlockMaterialInstance->SetScalarParameterValue(FName(TEXT("Opacity")), Value);
}
void AA_BlocksWallObstacle::DestroyTimelineFinished()
{
	CurrentBlock->BlockMeshComponent->DestroyComponent();
	CurrentBlock->BlockMeshComponent = nullptr;
	CurrentBlock = nullptr;
	AudioComponent->Stop();
}

void AA_BlocksWallObstacle::MoveBlockTimelineProgress(float Value)
{
}

void AA_BlocksWallObstacle::MoveBlockTimelineFinished()
{
	AudioComponent->Stop();
}

