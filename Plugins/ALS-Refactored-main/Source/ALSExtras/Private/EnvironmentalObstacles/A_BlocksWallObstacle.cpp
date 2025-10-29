#include "EnvironmentalObstacles/A_BlocksWallObstacle.h"
#include "Engine/StaticMesh.h"
#include "Components/AudioComponent.h"

AA_BlocksWallObstacle::AA_BlocksWallObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	MoveBlockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineComponent"));
}

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
			if (MeshComp->GetMaterial(0))
			{
				DynMat = MeshComp->CreateAndSetMaterialInstanceDynamic(0);
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

	//Timeline
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

}

void AA_BlocksWallObstacle::SetBlockState(FIntPoint GridPosition, EBlockState NewState)
{
	switch (NewState)
	{
	case EBlockState::Neutral:
	{
		SetBlockMaterial(GridPosition, false);
		break;
	}
	case EBlockState::Critical:
	{
		SetBlockMaterial(GridPosition, true);
		break;
	}
	case EBlockState::Destroyed:
	{
		break;
	}
	case EBlockState::Moving:
	{
		break;
	}
	case EBlockState::Falling:
	{
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

void AA_BlocksWallObstacle::StartBlockDestroy()
{
}

void AA_BlocksWallObstacle::MoveBlock()
{
}

void AA_BlocksWallObstacle::StartBlockFall()
{
}

void AA_BlocksWallObstacle::MoveBlockTimelineProgress(float Value)
{
}

void AA_BlocksWallObstacle::MoveBlockTimelineFinished()
{
}

