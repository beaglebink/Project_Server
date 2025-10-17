#include "Maze/A_Maze.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"

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

	MazeRenderTarget = NewObject<UTextureRenderTarget2D>(this);
	MazeRenderTarget->InitAutoFormat(256, 256);
	MazeRenderTarget->ClearColor = FLinearColor::Black;
	MazeRenderTarget->UpdateResourceImmediate();

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial && MazeTexture)
	{
		MeshDynamicMaterial->SetTextureParameterValue("Texture", MazeTexture);
		//MeshDynamicMaterial->SetTextureParameterValue("RenderTexture", MazeRenderTarget);
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
	DrawCellOnRenderTarget(CellX, CellY);
}

void AA_Maze::DrawCellOnRenderTarget(int32 CellX, int32 CellY)
{
	if (!MazeRenderTarget || !MeshDynamicMaterial)
	{
		return;
	}

	int32 RTWidth = MazeRenderTarget->SizeX;
	int32 RTHeight = MazeRenderTarget->SizeY;

	float U = CellX / (float)32;
	float V = CellY / (float)32;

	// Calculate cell position and size in render target space (doubled size for better visibility, that's why should do offset on half size)
	FVector2D CellSize(RTWidth / (float)32 * 2.0f, RTHeight / (float)32 * 2.0f);
	FVector2D CellPos(U * RTWidth - CellSize.X / 4, V * RTHeight - CellSize.Y / 4);

	FDrawToRenderTargetContext Context;
	UCanvas* Canvas;
	FVector2D Size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), MazeRenderTarget, Canvas, Size, Context);
	Canvas->K2_DrawTexture(BrushTexture, CellPos, CellSize, FVector2D(0, 0));

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);

	MeshDynamicMaterial->SetTextureParameterValue(FName("RenderTarget"), MazeRenderTarget);
}


void AA_Maze::OnFinishMaze()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Maze Finished!"));
}

