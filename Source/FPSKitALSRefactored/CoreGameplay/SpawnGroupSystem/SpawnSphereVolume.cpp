#include "SpawnSphereVolume.h"

ASpawnSphereVolume::ASpawnSphereVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    SphereComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
    SphereComponent->SetupAttachment(RootComponent);
    SphereComponent->SetSphereRadius(SphereRadius);
    SphereComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SphereComponent->SetVisibility(true);
}

void ASpawnSphereVolume::SetSphereRadius(float Radius)
{
    SphereRadius = Radius;
    if (SphereComponent)
    {
        SphereComponent->SetSphereRadius(SphereRadius);
    }
}

FVector ASpawnSphereVolume::GetRandomSpawnPoint_Implementation() const
{
    const FVector Origin = GetActorLocation();
    const FVector RandomDir = FMath::VRand();
    const float RandomRadius = FMath::FRandRange(0.0f, SphereRadius);
    return Origin + RandomDir * RandomRadius;
}

FRotator ASpawnSphereVolume::GetDefaultRotation_Implementation() const
{
    return GetActorRotation();
}