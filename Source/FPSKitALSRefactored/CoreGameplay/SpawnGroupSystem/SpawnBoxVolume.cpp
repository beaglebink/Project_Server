#include "SpawnBoxVolume.h"

ASpawnBoxVolume::ASpawnBoxVolume()
{
    PrimaryActorTick.bCanEverTick = false;
    
    BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
    BoxComponent->SetupAttachment(RootComponent);
    BoxComponent->SetBoxExtent(BoxExtent);
    BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BoxComponent->SetVisibility(true);
}

void ASpawnBoxVolume::SetBoxExtent(const FVector& Extent)
{
    BoxExtent = Extent;
    if (BoxComponent)
    {
        BoxComponent->SetBoxExtent(BoxExtent);
    }
}

FVector ASpawnBoxVolume::GetRandomSpawnPoint_Implementation() const
{
    const FVector Origin = GetActorLocation();
    return Origin + FVector(
        FMath::FRandRange(-BoxExtent.X, BoxExtent.X),
        FMath::FRandRange(-BoxExtent.Y, BoxExtent.Y),
        FMath::FRandRange(-BoxExtent.Z, BoxExtent.Z)
    );
}

FRotator ASpawnBoxVolume::GetRandomRotation_Implementation() const
{
    return GetActorRotation();
}