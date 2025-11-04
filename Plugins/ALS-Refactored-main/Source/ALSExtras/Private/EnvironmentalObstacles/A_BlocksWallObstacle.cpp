#include "EnvironmentalObstacles/A_BlocksWallObstacle.h"
#include "EnvironmentalObstacles/A_BlockObstacle.h"
#include "Components/TextRenderComponent.h"

AA_BlocksWallObstacle::AA_BlocksWallObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
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
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_BlocksWallObstacle, NumberOfBlocksDestroyedThreshold))
	{
		NumberOfBlocksDestroyedThreshold = FMath::Clamp(NumberOfBlocksDestroyedThreshold, 1, FMath::RoundToInt(WallDimensions.X * WallDimensions.Y * CriticalBlocksTreshold / 100.f) - 1);
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_BlocksWallObstacle, bUsePlayerAccuracy))
	{
		if (bUsePlayerAccuracy)
		{
			bUseTimeInterval = false;
			bUseNumberOfBlocksDestroyed = false;
		}
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(AA_BlocksWallObstacle, PlayerAccuracyThreshold))
	{
		PlayerAccuracyThreshold = FMath::Clamp(PlayerAccuracyThreshold, 1, FMath::RoundToInt(WallDimensions.X * WallDimensions.Y * CriticalBlocksTreshold / 100.f) - 1);
	}
}
#endif

void AA_BlocksWallObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	for (AA_BlockObstacle* Block : WallBlocks)
	{
		if (Block && !Block->IsPendingKillPending())
		{
			Block->Destroy();
		}
	}
	WallBlocks.Empty();

	//BlockExtent
	if (BlockObstacleClass)
	{
		if (AA_BlockObstacle* TempBlock = GetWorld()->SpawnActorDeferred<AA_BlockObstacle>(BlockObstacleClass, FTransform()))
		{
			BlockExtent = TempBlock->BlockMeshComponent->GetStaticMesh()->GetExtendedBounds().BoxExtent * 2.0f;;
			TempBlock->Destroy();
		}
	}

	//Blocks spawning
	for (int32 Y = 0; Y < WallDimensions.Y; ++Y)
	{
		for (int32 X = 0; X < WallDimensions.X; ++X)
		{
			FVector BlockLocation = GetActorLocation() - X * BlockExtent.Y * GetActorRightVector() - Y * BlockExtent.Z * GetActorUpVector();
			FTransform BlockTransform(GetActorRotation(), BlockLocation);
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AA_BlockObstacle* NewBlock = GetWorld()->SpawnActor<AA_BlockObstacle>(BlockObstacleClass, BlockTransform, SpawnParams))
			{
				NewBlock->InitialLocation = BlockLocation;
				NewBlock->TargetLocation = BlockLocation;
				NewBlock->BlockIndex = Y * WallDimensions.X + X;
				WallBlocks.Add(NewBlock);
			}
		}
	}

	//Set critical blocks
	int32 TotalBlocks = WallBlocks.Num();
	int32 NumCritical = FMath::RoundToInt(TotalBlocks * CriticalBlocksTreshold / 100.f);

	TArray<int32> BlockIndices;
	BlockIndices.Reserve(TotalBlocks);
	for (int32 i = 0; i < TotalBlocks; ++i)
	{
		BlockIndices.Add(i);
	}

	for (int32 i = 0; i < BlockIndices.Num(); ++i)
	{
		int32 SwapIndex = FMath::RandRange(0, BlockIndices.Num() - 1);
		BlockIndices.Swap(i, SwapIndex);
	}

	for (int32 i = 0; i < NumCritical; ++i)
	{
		int32 Idx = BlockIndices[i];
		if (WallBlocks.IsValidIndex(Idx))
		{
			WallBlocks[Idx]->SetBlockState(EBlockState::Critical);
		}
	}
}

void AA_BlocksWallObstacle::BeginPlay()
{
	Super::BeginPlay();

	if (bUseTimeInterval)
	{
		FTimerHandle SwapHandle;
		GetWorldTimerManager().SetTimer(SwapHandle, this, &AA_BlocksWallObstacle::PrepareBlockSwaps, TimeIntervalBetweenSwaps, true, TimeIntervalBetweenSwaps);
	}
}

void AA_BlocksWallObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_BlocksWallObstacle::PrepareBlockSwaps()
{
	bIsProcessingSwaps = true;

	const int32 BlinkCount = 5;
	const float BlinkInterval = 0.8f;

	for (int32 i = 1; i <= BlinkCount; ++i)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
			{
				for (AA_BlockObstacle* Block : WallBlocks)
				{
					if (IsValid(Block))
					{
						Block->HandleShotForMaterial();
					}
				}
			}, i * (BlinkInterval + 0.2f), false);
	}

	FTimerHandle ProcessHandle;
	GetWorldTimerManager().SetTimer(ProcessHandle, this, &AA_BlocksWallObstacle::ProcessBlockSwaps, BlinkCount * (BlinkInterval + 0.2f), false);
}


void AA_BlocksWallObstacle::ProcessBlockSwaps()
{
	if (WallBlocks.Num() == 0)
		return;

	TSet<int32> UsedNeutralIndices;

	int32 StartIndex = FMath::RandRange(0, WallBlocks.Num() - 1);

	for (int32 Offset = 0; Offset < WallBlocks.Num(); ++Offset)
	{
		int32 i = (StartIndex + Offset) % WallBlocks.Num();

		if (!IsValid(WallBlocks[i]) || WallBlocks[i]->BlockState != EBlockState::Critical || UsedNeutralIndices.Contains(i))
		{
			continue;
		}

		int32 StartDirIndex = FMath::RandRange(0, 3);

		int32 BestCandidate = -1;
		EDirection BestCandidateDirection = EDirection::Up;
		int32 MinCriticalNeighbors = 4;

		for (int32 DirOffset = 0; DirOffset < 4; ++DirOffset)
		{
			int32 DirIndex = (StartDirIndex + DirOffset) % 4;
			EDirection Dir = static_cast<EDirection>(DirIndex);

			int32 NeighborIndex = GetNeighborIndex(i, Dir);
			if (NeighborIndex == -1 || !IsValid(WallBlocks[NeighborIndex]) || WallBlocks[NeighborIndex]->BlockState != EBlockState::Neutral || UsedNeutralIndices.Contains(NeighborIndex))
			{
				continue;
			}

			int32 CriticalCount = 0;
			static const TArray<EDirection> Directions = { EDirection::Up, EDirection::Down, EDirection::Left, EDirection::Right };

			for (EDirection CheckDir : Directions)
			{
				if (IsOppositeDirection(CheckDir, Dir))
				{
					continue;
				}

				int32 SubNeighborIndex = GetNeighborIndex(NeighborIndex, CheckDir);
				if (SubNeighborIndex != -1 && IsValid(WallBlocks[SubNeighborIndex]) && WallBlocks[SubNeighborIndex]->BlockState == EBlockState::Critical)
				{
					++CriticalCount;
				}
			}

			if (CriticalCount < MinCriticalNeighbors)
			{
				MinCriticalNeighbors = CriticalCount;
				BestCandidate = NeighborIndex;
				BestCandidateDirection = Dir;
			}

			if (CriticalCount == 0)
			{
				break;
			}
		}

		if (BestCandidate != -1)
		{
			UsedNeutralIndices.Add(i);
			UsedNeutralIndices.Add(BestCandidate);

			WallBlocks[i]->MoveOnDirectionBlock(BestCandidateDirection);
			++BlocksOnSwapCount;
			WallBlocks[BestCandidate]->MoveOnDirectionBlock(GetOppositeDirection(BestCandidateDirection));
			++BlocksOnSwapCount;

			AA_BlockObstacle* TempBlock = WallBlocks[i];
			WallBlocks[i] = WallBlocks[BestCandidate];
			WallBlocks[BestCandidate] = TempBlock;

			WallBlocks[i]->BlockIndex = i;
			WallBlocks[BestCandidate]->BlockIndex = BestCandidate;
		}
	}
}


int32 AA_BlocksWallObstacle::GetNeighborIndex(int32 Index, EDirection Direction) const
{
	const int32 Col = Index % WallDimensions.X;      // X
	const int32 Row = Index / WallDimensions.X;      // Y

	switch (Direction)
	{
	case EDirection::Up:
	{
		int32 NewRow = Row - 1;
		if (NewRow < 0)
		{
			return -1;
		}
		return NewRow * WallDimensions.X + Col;
	}
	case EDirection::Down:
	{
		int32 NewRow = Row + 1;
		if (NewRow >= WallDimensions.Y) return -1;
		return NewRow * WallDimensions.X + Col;
	}
	case EDirection::Left:
	{
		if (Col == 0)
		{
			return -1;
		}
		return Index - 1;
	}
	case EDirection::Right:
	{
		if (Col == WallDimensions.X - 1) return -1;
		return Index + 1;
	}
	default:
		return -1;
	}
}

bool AA_BlocksWallObstacle::IsOppositeDirection(EDirection A, EDirection B) const
{
	return (A == EDirection::Up && B == EDirection::Down) ||
		(A == EDirection::Down && B == EDirection::Up) ||
		(A == EDirection::Left && B == EDirection::Right) ||
		(A == EDirection::Right && B == EDirection::Left);
}

EDirection AA_BlocksWallObstacle::GetOppositeDirection(EDirection Direction) const
{
	switch (Direction)
	{
	case EDirection::Up:
		return EDirection::Down;
	case EDirection::Down:
		return EDirection::Up;
	case EDirection::Left:
		return EDirection::Right;
	case EDirection::Right:
		return EDirection::Left;
	default:
		return Direction;
	}
}

void AA_BlocksWallObstacle::CheckAndHandleCompletedSwaps()
{
	if (--BlocksOnSwapCount == 0)
	{
		bIsProcessingSwaps = false;
	}
}

void AA_BlocksWallObstacle::UpperBlocksFall(int32 Index)
{
	int32 PrevIndex = Index;
	WallBlocks[PrevIndex] = nullptr;
	AA_BlockObstacle* PrevBlock = nullptr;

	for (int32 i = Index - WallDimensions.X; i >= 0; i -= WallDimensions.X)
	{
		if (WallBlocks[i] && WallBlocks.IsValidIndex(i))
		{
			WallBlocks[PrevIndex] = WallBlocks[i];
			WallBlocks[PrevIndex]->BlockIndex = PrevIndex;
			WallBlocks[PrevIndex]->StartBlockFall(PrevBlock);
			PrevBlock = WallBlocks[PrevIndex];
			WallBlocks[i] = nullptr;
			PrevIndex = i;
		}
	}
	DrawGrid();
}

void AA_BlocksWallObstacle::DrawGrid()
{
#if WITH_EDITOR
	for (UTextRenderComponent* Text : DebugGridTexts)
	{
		Text->DestroyComponent();
	}
	DebugGridTexts.Empty();

	for (int32 i = 0; i < WallBlocks.Num(); ++i)
	{
		FVector TextLocation = GetActorLocation() + GetActorUpVector() * 200.0f - GetActorUpVector() * BlockExtent.Z * i / WallDimensions.X - GetActorRightVector() * BlockExtent.Y * (i % WallDimensions.X);
		DrawDebugSphere(GetWorld(), TextLocation, 10.0f, 8, WallBlocks[i] && WallBlocks[i]->LowerBlock ? FColor::Green : FColor::Red, false, 20.0f);

		UTextRenderComponent* TextComp = NewObject<UTextRenderComponent>(this);
		if (TextComp)
		{
			TextComp->RegisterComponent();
			TextComp->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			TextComp->SetWorldLocation(TextLocation);
			TextComp->SetHorizontalAlignment(EHTA_Center);
			TextComp->SetVerticalAlignment(EVRTA_TextCenter);
			TextComp->SetWorldSize(10.f);
			TextComp->SetTextRenderColor(FColor::Blue);
			//TextComp->SetText(FText::FromString(FString::FromInt(static_cast<int32>(IsValid(WallBlocks[i]) ? WallBlocks[i]->BlockState : EBlockState::Destroyed))));
			//TextComp->SetText(FText::FromString(FString::FromInt(IsValid(WallBlocks[i]) ? WallBlocks[i]->BlockIndex : -1)));
			TextComp->SetText(FText::FromString(FString::FromInt(IsValid(WallBlocks[i]) ? WallBlocks[i]->InitialLocation.Z : -1)));
			DebugGridTexts.Add(TextComp);
		}
	}
#endif
}
