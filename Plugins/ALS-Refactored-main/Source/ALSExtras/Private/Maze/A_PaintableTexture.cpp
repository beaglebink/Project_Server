#include "Maze/A_PaintableTexture.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"

AA_PaintableTexture::AA_PaintableTexture()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));

	StaticMeshComponent->SetupAttachment(RootComponent);
}

void AA_PaintableTexture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RenderTarget = NewObject<UTextureRenderTarget2D>(this);
	RenderTarget->InitAutoFormat(256, 256);
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->UpdateResourceImmediate();

	if (!MeshDynamicMaterial)
	{
		MeshDynamicMaterial = StaticMeshComponent->CreateAndSetMaterialInstanceDynamic(0);
	}
	if (MeshDynamicMaterial && PaintableTexture)
	{
		MeshDynamicMaterial->SetTextureParameterValue("PaintableTexture", PaintableTexture);
		MeshDynamicMaterial->SetTextureParameterValue("RenderTexture", RenderTarget);

		ReadTextureToArray();
	}
}

void AA_PaintableTexture::BeginPlay()
{
	Super::BeginPlay();
}

void AA_PaintableTexture::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_PaintableTexture::ReadTextureToArray()
{
	TArray<FColor> Pixels;
	int32 Width = PaintableTexture->GetSizeX();
	int32 Height = PaintableTexture->GetSizeY();

	FTexture2DMipMap& Mip = PaintableTexture->GetPlatformData()->Mips.Last();
	void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);

	Pixels.SetNumUninitialized(Width * Height);
	FMemory::Memcpy(Pixels.GetData(), Data, Width * Height * sizeof(FColor));

	Mip.BulkData.Unlock();

	PixelArray.SetNum(Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		PixelArray[Y].Row.SetNum(Width);
		for (int32 X = 0; X < Width; ++X)
		{
			FColor Pixel = Pixels[Y * Width + X];
			if (Pixel.R < 100 && Pixel.G < 100 && Pixel.B < 100)
			{
				PixelArray[Y].Row[X] = -1; // Wall
			}
			else if (Pixel.G > 100 && Pixel.R < 100 && Pixel.B < 100)
			{
				PixelArray[Y].Row[X] = 1; // Start
			}
			else if (Pixel.B > 100 && Pixel.R < 100 && Pixel.G < 100)
			{
				PixelArray[Y].Row[X] = 2; // Finish
			}
			else
			{
				PixelArray[Y].Row[X] = 0; // Wayable
			}
		}
	}
}

void AA_PaintableTexture::DrawCellOnRenderTarget(int32 CellX, int32 CellY)
{
	if (!RenderTarget || !MeshDynamicMaterial)
	{
		return;
	}

	int32 RTWidth = RenderTarget->SizeX;
	int32 RTHeight = RenderTarget->SizeY;

	float U = CellX / (float)32;
	float V = CellY / (float)32;

	// Calculate cell position and size in render target space (doubled size for better visibility, that's why should do offset on half size)
	FVector2D CellSize(RTWidth / (float)32 * 2.0f, RTHeight / (float)32 * 2.0f);
	FVector2D CellPos(U * RTWidth - CellSize.X / 4, V * RTHeight - CellSize.Y / 4);

	FDrawToRenderTargetContext Context;
	UCanvas* Canvas;
	FVector2D Size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(GetWorld(), RenderTarget, Canvas, Size, Context);
	Canvas->K2_DrawTexture(BrushTexture, CellPos, CellSize, FVector2D(0, 0));

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(GetWorld(), Context);

	MeshDynamicMaterial->SetTextureParameterValue(FName("RenderTarget"), RenderTarget);
}

void AA_PaintableTexture::OnFinish()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Finished!"));
}

