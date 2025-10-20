#include "Maze/A_BlackTile.h"

AA_BlackTile::AA_BlackTile()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_BlackTile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_BlackTile::BeginPlay()
{
	Super::BeginPlay();
}

void AA_BlackTile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_BlackTile::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	FVector ImpactLocation = Hit.ImpactPoint - GetActorLocation();

	float NormalizedX = -ImpactLocation.Y / StaticMeshComponent->Bounds.BoxExtent.Y / 2.0f;
	float NormalizedY = -ImpactLocation.Z / StaticMeshComponent->Bounds.BoxExtent.Z / 2.0f;

	int32 CellX = FMath::Clamp(FMath::FloorToInt(NormalizedX * 32), 0, 31);
	int32 CellY = FMath::Clamp(FMath::FloorToInt(NormalizedY * 32), 0, 31);

	PaintCell(CellX, CellY);
}

void AA_BlackTile::PaintCell(int32 CellX, int32 CellY)
{
	const int32 Height = PixelArray.Num();
	const int32 Width = PixelArray[0].Row.Num();

	if (!PixelArray.IsValidIndex(CellY) || !PixelArray[CellY].Row.IsValidIndex(CellX))
	{
		return;
	}

	int32& Cell = PixelArray[CellY].Row[CellX];

	const TArray<FIntPoint> Neighbors =
	{
		{CellX + 1, CellY}, {CellX - 1, CellY},
		{CellX, CellY + 1}, {CellX, CellY - 1}
	};

	if (bIsFirstPixel)
	{
		const bool bIsOnBorder = (CellX == 0 || CellX == Width - 1 || CellY == 0 || CellY == Height - 1);
		if (!bIsOnBorder)
		{
			return;
		}
		bIsFirstPixel = false;
	}
	else
	{
		if (!HasColoredOutline())
		{
			const bool bIsOnBorder = (CellX == 0 || CellX == Width - 1 || CellY == 0 || CellY == Height - 1);
			if (!bIsOnBorder)
			{
				return;
			}

			bool bHasGreenNeighbor = false;
			for (const FIntPoint& N : Neighbors)
			{
				if (N.X >= 0 && N.X < Width && N.Y >= 0 && N.Y < Height)
				{
					if (PixelArray[N.Y].Row[N.X] == 1)
					{
						bHasGreenNeighbor = true;
						break;
					}
				}
			}

			if (!bHasGreenNeighbor)
			{
				return;
			}
		}
	}

	const int32 ThresholdCount = FMath::CeilToInt(Width * Height * PaintedPercentThreshold / 100.0f);

	if (PaintedPixelCount >= ThresholdCount)
	{
		OnFinish();
		return;
	}

	if (Cell != 1)
	{
		++PaintedPixelCount;
		Cell = 1;
	}

	DrawCellOnRenderTarget(CellX, CellY);
}

bool AA_BlackTile::HasColoredOutline()
{
	const int32 Height = PixelArray.Num();
	const int32 Width = PixelArray[0].Row.Num();

	for (int32 y = 0; y < Height; ++y)
	{
		if (PixelArray[y].Row[0] != 1 || PixelArray[y].Row[Width - 1] != 1)
		{
			return false;
		}
	}

	for (int32 x = 0; x < Width; ++x)
	{
		if (PixelArray[0].Row[x] != 1 || PixelArray[Height - 1].Row[x] != 1)
		{
			return false;
		}
	}

	return true;
}

void AA_BlackTile::OnFinish()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Painting Finished!"));
}

