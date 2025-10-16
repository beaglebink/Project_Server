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

	//TArray<FColor> Pixels;
	//int32 Width = MazeTexture->GetSizeX();
	//int32 Height = MazeTexture->GetSizeY();

	//FTexture2DMipMap& Mip = MazeTexture->GetPlatformData()->Mips.Last();
	//void* Data = Mip.BulkData.Lock(LOCK_READ_ONLY);

	//Pixels.SetNumUninitialized(Width * Height);
	//FMemory::Memcpy(Pixels.GetData(), Data, Width * Height * sizeof(FColor));

	//Mip.BulkData.Unlock();

	//MazeArray.SetNum(Height);
	//for (int32 Y = 0; Y < Height; ++Y)
	//{
	//	MazeArray[Y].Row.SetNum(Width);
	//	for (int32 X = 0; X < Width; ++X)
	//	{
	//		FColor Pixel = Pixels[Y * Width + X];
	//		MazeArray[Y].Row[X] = (Pixel.R < 128) ? -1 : 0;
	//	}
	//}
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
	if (!MazeTexture || !MazeRenderTarget || !MeshDynamicMaterial)
	{
		return;
	}

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(), MazeRenderTarget, MeshDynamicMaterial);

	TArray<FLinearColor> Pixels;
	UKismetRenderingLibrary::ReadRenderTargetRaw(GetWorld(), MazeRenderTarget, Pixels);

	int32 Width = MazeRenderTarget->SizeX;
	int32 Height = MazeRenderTarget->SizeY;

	MazeArray.SetNum(Height);
	for (int32 Y = 0; Y < Height; ++Y)
	{
		MazeArray[Y].Row.SetNum(Width);
		for (int32 X = 0; X < Width; ++X)
		{
			FLinearColor Pixel = Pixels[Y * Width + X];
			MazeArray[Y].Row[X] = (Pixel.R < 0.5f) ? -1 : 0;
		}
	}
}

void AA_Maze::HandleWeaponShot_Implementation(UPARAM(ref)FHitResult& Hit)
{
	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.0f, 8, FColor::Green, true, 5.0f, 0u, 2.0f);
}

