#include "Maze/A_Maze.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetRenderingLibrary.h"

AA_Maze::AA_Maze()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	StaticMeshComponent->SetupAttachment(RootComponent);
}

void AA_Maze::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial && MazeTexture)
	{
		MeshDynamicMaterial->SetTextureParameterValue("Texture", MazeTexture);
	}

	ReadMazeTextureToArray();
}

void AA_Maze::BeginPlay()
{
	Super::BeginPlay();
}

void AA_Maze::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	for (size_t i = 0; i < MazeArray.Num(); i++)
	{
		for (size_t j = 0; j < MazeArray[0].Row.Num(); j++)
		{
			DrawDebugSphere(GetWorld(), FVector(GetActorLocation().X + 50.0f, GetActorLocation().Y - j * 5.0f-500.0f, GetActorLocation().Z - i * 5.0f), 2.0f, 4, MazeArray[i].Row[j] == -1 ? FColor::Black : MazeArray[i].Row[j] == 0 ? FColor::White : MazeArray[i].Row[j] == 1 ? FColor::Green : FColor::Blue, false, 0.1f, 0u, 1.0f);
		}
	}
}

void AA_Maze::ReadMazeTextureToArray()
{
	TArray<FColor> Pixels;
	int32 Width = MazeTexture->GetSizeX();
	int32 Height = MazeTexture->GetSizeY();

	FTexture2DMipMap& Mip = MazeTexture->GetPlatformData()->Mips.Last();
	void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);

	Pixels.SetNumUninitialized(Width * Height);
	FMemory::Memcpy(Pixels.GetData(), Data, Width * Height * sizeof(FColor));

	Mip.BulkData.Unlock();

	MazeArray.SetNum(Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		MazeArray[Y].Row.SetNum(Width);
		for (int32 X = 0; X < Width; ++X)
		{
			FColor Pixel = Pixels[Y * Width + X];
			if (Pixel.R < 100 && Pixel.G < 100 && Pixel.B < 100)
			{
				MazeArray[Y].Row[X] = -1; // Wall
			}
			else if (Pixel.G > 100 && Pixel.R < 100 && Pixel.B < 100)
			{
				MazeArray[Y].Row[X] = 1; // Start
			}
			else if (Pixel.B > 100 && Pixel.R < 100 && Pixel.G < 100)
			{
				MazeArray[Y].Row[X] = 2; // Finish
			}
			else
			{
				MazeArray[Y].Row[X] = 0; // Wayable
			}
		}
	}
}

void AA_Maze::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	FVector ImpactLocation = Hit.ImpactPoint - GetActorLocation();

	float NormalizedX = -ImpactLocation.Y / StaticMeshComponent->Bounds.BoxExtent.Y / 2.0f;
	float NormalizedY = -ImpactLocation.Z / StaticMeshComponent->Bounds.BoxExtent.Z / 2.0f;

	int32 CellX = FMath::Clamp(FMath::FloorToInt(NormalizedX * 32), 0, 31);
	int32 CellY = FMath::Clamp(FMath::FloorToInt(NormalizedY * 32), 0, 31);

	PaintCell(CellX, CellY);
}

void AA_Maze::PaintCell(int32 CellX, int32 CellY)
{
	if (!MazeArray.IsValidIndex(CellY) || !MazeArray[CellY].Row.IsValidIndex(CellX))
	{
		return;
	}

	int32& Cell = MazeArray[CellY].Row[CellX];

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
		if (MazeArray.IsValidIndex(N.Y) && MazeArray[N.Y].Row.IsValidIndex(N.X))
		{
			if (MazeArray[N.Y].Row[N.X] == 1)
			{
				bHasGreenNeighbor = true;
			}
			if (MazeArray[N.Y].Row[N.X] == 2)
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
		OnFinishMaze();
	}

	Cell = 1;
}

void AA_Maze::OnFinishMaze()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Maze Finished!"));
}

