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

	if (bIsFirstPixel)
	{
		const bool bIsOnBorder = (CellX == 0 || CellX == Width - 1 || CellY == 0 || CellY == Height - 1);
		if (!bIsOnBorder)
		{
			return;
		}
		bIsFirstPixel = false;
	}
	else if (!HasColoredOutline())
	{
		const int32 Spread = 4;
		bool bFound = false;
		FIntPoint CellToPaint = { CellX, CellY };

		for (int32 y = FMath::Max(0, CellY - Spread); y <= FMath::Min(Height - 1, CellY + Spread) && !bFound; ++y)
		{
			for (int32 x = FMath::Max(0, CellX - Spread); x <= FMath::Min(Width - 1, CellX + Spread) && !bFound; ++x)
			{
				if (PixelArray[y].Row[x] == 1)
				{
					const TArray<FIntPoint> Neighbors =
					{
						{x + 1, y}, {x - 1, y},
						{x, y + 1}, {x, y - 1}
					};
					for (const FIntPoint& N : Neighbors)
					{
						const bool bIsOnBorder = (N.X == 0 || N.X == Width - 1 || N.Y == 0 || N.Y == Height - 1);
						if (PixelArray.IsValidIndex(N.Y) && PixelArray[N.Y].Row.IsValidIndex(N.X) && bIsOnBorder && PixelArray[N.Y].Row[N.X] == -1)
						{
							bFound = true;
							CellToPaint.X = N.X;
							CellToPaint.Y = N.Y;
							break;
						}
					}
				}
			}
		}
		if (!bFound)
		{
			return;
		}
		CellX = CellToPaint.X;
		CellY = CellToPaint.Y;
	}
	const int32 ThresholdCount = FMath::CeilToInt(Width * Height * PaintedPercentThreshold / 100.0f);

	if (PaintedPixelCount >= ThresholdCount)
	{
		OnFinish();
		return;
	}

	if (PixelArray[CellY].Row[CellX] != 1)
	{
		++PaintedPixelCount;
		PixelArray[CellY].Row[CellX] = 1;
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

