#include "EnvironmentalHazard/A_OrderSystem.h"
#include "EnvironmentalHazard/A_OrderCube.h"
#include "Kismet/GameplayStatics.h"

AA_OrderSystem::AA_OrderSystem()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

}

void AA_OrderSystem::OnConstruction(const FTransform& Transform)
{
	if (!GetWorld() || GetWorld()->IsGameWorld())
	{
		return;
	}

	Super::OnConstruction(Transform);

	DestroyCubes();

#if WITH_EDITOR
	if (OrderCubeClass)
	{
		for (size_t i = 1; i <= CubesQuantity; ++i)
		{
			FVector Location = GetActorLocation() + FVector(i * 200.0f, 0.0f, 0.0f);
			FRotator Rotation = FRotator::ZeroRotator;

			FTransform SpawnTransform(Rotation, Location);

			AA_OrderCube* OrderCube = Cast<AA_OrderCube>(UGameplayStatics::BeginDeferredActorSpawnFromClass(GetWorld(), OrderCubeClass, SpawnTransform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, this));

			if (OrderCube)
			{
				OrderCube->SetOwner(this);
				OrderCube->CubeIndex = i;
				OrderCube->DamageCaused = Damage;

				UGameplayStatics::FinishSpawningActor(OrderCube, SpawnTransform);

				OrderCube->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
				CubesArray.Add(OrderCube);
			}
		}
	}
#endif
}

void AA_OrderSystem::BeginPlay()
{
	Super::BeginPlay();
}

void AA_OrderSystem::Destroyed()
{
	Super::Destroyed();

	DestroyCubes();
}
void AA_OrderSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_OrderSystem::CheckHitCubeRightOrderIndex(AA_OrderCube* OrderCube)
{
	if ((PrevIndex + 1) == OrderCube->CubeIndex)
	{
		++PrevIndex;
		OrderCube->MeshDynamicMaterial->SetScalarParameterValue("Brightness", 30);
	}
	else
	{
		PrevIndex = 0;
		for (AA_OrderCube* Cube : CubesArray)
		{
			Cube->MeshDynamicMaterial->SetScalarParameterValue("Brightness", 3);
		}
	}
	if (PrevIndex == CubesQuantity)
	{
		for (AA_OrderCube* Cube : CubesArray)
		{
			Cube->OnDeath();
		}

		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(TimerHandle, [this]()
			{
				Destroy();
			}, 3.5f, false);
	}
}

void AA_OrderSystem::DestroyCubes()
{
	for (AA_OrderCube* Cube : CubesArray)
	{
		if (IsValid(Cube))
		{
			Cube->Destroy();
		}
	}

	CubesArray.Empty();
}