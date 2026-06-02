#include "SpawnVolume.h"

ASpawnVolume::ASpawnVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector ASpawnVolume::GetRandomSpawnPoint_Implementation() const
{
    return GetActorLocation();
}

FRotator ASpawnVolume::GetDefaultRotation_Implementation() const
{
    return GetActorRotation();
}