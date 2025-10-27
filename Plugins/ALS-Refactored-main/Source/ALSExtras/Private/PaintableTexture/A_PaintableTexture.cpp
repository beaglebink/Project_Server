#include "PaintableTexture/A_PaintableTexture.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/Canvas.h"
#include "Components/AudioComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AA_PaintableTexture::AA_PaintableTexture()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("TimelineComponent"));

	StaticMeshComponent->SetupAttachment(RootComponent);
	AudioComponent->SetupAttachment(StaticMeshComponent);

	AudioComponent->bAutoActivate = false;
}

void AA_PaintableTexture::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (!RenderTarget)
	{
		RenderTarget = NewObject<UTextureRenderTarget2D>(this);
		RenderTarget->InitAutoFormat(256, 256);
		RenderTarget->ClearColor = FLinearColor::Black;
		RenderTarget->UpdateResourceImmediate();
	}

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

	if (DissolveFloatCurve)
	{
		DissolveProgressFunction.BindUFunction(this, FName("DissolveTimelineProgress"));
		DissolveTimeline->AddInterpFloat(DissolveFloatCurve, DissolveProgressFunction);

		DissolveFinishedFunction.BindUFunction(this, FName("DissolveTimelineFinished"));
		DissolveTimeline->SetTimelineFinishedFunc(DissolveFinishedFunction);

		DissolveTimeline->SetLooping(false);
	}
}

void AA_PaintableTexture::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Debug draw pixel array
	//for (size_t i = 0; i < PixelArray.Num(); i++)
	//{
	//	for (size_t j = 0; j < PixelArray[0].Row.Num(); j++)
	//	{
	//		DrawDebugSphere(GetWorld(), FVector(GetActorLocation().X, GetActorLocation().Y + 200 - j * 5, GetActorLocation().Z - i * 5), 5, 4, PixelArray[i].Row[j] == -1 ? FColor::Black : PixelArray[i].Row[j] == 0 ? FColor::White : PixelArray[i].Row[j] == 1 ? FColor::Green : FColor::Blue, false, 0.1);
	//	}
	//}
}

void AA_PaintableTexture::ReadTextureToArray()
{
	TArray<FColor> Pixels;
	int32 Width = PaintableTexture->GetSizeX();
	int32 Height = PaintableTexture->GetSizeY();

	FTexturePlatformData* PlatformData = PaintableTexture->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		return;
	}

	FTexture2DMipMap& Mip = PlatformData->Mips.Last();
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
	bIsOnDissolving = true;

	// Dissolve VFX
	if (UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAttached(NiagaraSystem, StaticMeshComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true, false))
	{
		NiagaraComp->ActivateSystem(true);
	}

	AudioComponent->Play();
	DissolveTimeline->Play();
}

void AA_PaintableTexture::DissolveTimelineProgress(float Value)
{
	MeshDynamicMaterial->SetScalarParameterValue(FName("Opacity"), Value);

}

void AA_PaintableTexture::DissolveTimelineFinished()
{
	AudioComponent->Stop();
}

