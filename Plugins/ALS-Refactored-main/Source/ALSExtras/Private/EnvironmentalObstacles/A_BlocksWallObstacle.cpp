#include "EnvironmentalObstacles/A_BlocksWallObstacle.h"

AA_BlocksWallObstacle::AA_BlocksWallObstacle()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_BlocksWallObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_BlocksWallObstacle::BeginPlay()
{
	Super::BeginPlay();
}

void AA_BlocksWallObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

