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


}

void AA_BlocksWallObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
		DrawDebugSphere(GetWorld(), TextLocation, 10.0f, 8, WallBlocks[i] && WallBlocks[i]->LowerBlock? FColor::Green : FColor::Red, false, 20.0f);

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
			TextComp->SetText(FText::FromString(FString::FromInt(IsValid(WallBlocks[i]) ? WallBlocks[i]->BlockIndex : -1)));
			DebugGridTexts.Add(TextComp);
		}
	}
#endif
}
