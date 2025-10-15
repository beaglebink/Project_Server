#include "Maze/A_Maze.h"

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
	if (MeshDynamicMaterial)
	{
		MeshDynamicMaterial->SetTextureParameterValue("Texture", MazeTexture);
	}
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
	DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 5.0f, 8, FColor::Green, true, 5.0f, 0u, 2.0f);
}

