#include "Maze/A_Maze.h"

AA_Maze::AA_Maze()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_Maze::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_Maze::BeginPlay()
{
	Super::BeginPlay();
}

void AA_Maze::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_Maze::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	if (bIsOnDissolving)
	{
		return;
	}

	FVector ImpactLocation = Hit.ImpactPoint - GetActorLocation();

	float NormalizedX = -ImpactLocation.Y / StaticMeshComponent->Bounds.BoxExtent.Y / 2.0f;
	float NormalizedY = -ImpactLocation.Z / StaticMeshComponent->Bounds.BoxExtent.Z / 2.0f;

	int32 CellX = FMath::Clamp(FMath::FloorToInt(NormalizedX * 32), 0, 31);
	int32 CellY = FMath::Clamp(FMath::FloorToInt(NormalizedY * 32), 0, 31);

	PaintCell(CellX, CellY);
}

void AA_Maze::PaintCell(int32 CellX, int32 CellY)
{
	if (!PixelArray.IsValidIndex(CellY) || !PixelArray[CellY].Row.IsValidIndex(CellX))
	{
		return;
	}

	int32& Cell = PixelArray[CellY].Row[CellX];

	if (Cell == -1)
	{
		return;
	}

	const TArray<FIntPoint> Neighbors =
	{
		{CellX + 1, CellY}, {CellX - 1, CellY},
		{CellX, CellY + 1}, {CellX, CellY - 1}
	};

	bool bHasGreenNeighbor = false;
	bool bHasBlueNeighbor = false;

	for (const FIntPoint& N : Neighbors)
	{
		if (PixelArray.IsValidIndex(N.Y) && PixelArray[N.Y].Row.IsValidIndex(N.X))
		{
			if (PixelArray[N.Y].Row[N.X] == 1)
			{
				bHasGreenNeighbor = true;
			}
			if (PixelArray[N.Y].Row[N.X] == 2)
			{
				bHasBlueNeighbor = true;
			}
		}
	}

	if (!bHasGreenNeighbor)
	{
		return;
	}

	if (bHasBlueNeighbor)
	{
		OnFinish();
	}

	Cell = 1;
	DrawCellOnRenderTarget(CellX, CellY);
}