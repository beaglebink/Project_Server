#include "DebrisSystem/A_EnemyBase.h"
#include "DebrisSystem/A_Debris.h"

AA_EnemyBase::AA_EnemyBase()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));

	MeshComponent->SetupAttachment(RootComponent);
}

void AA_EnemyBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_EnemyBase::BeginPlay()
{
	Super::BeginPlay();

	FTimerHandle TimerHandle;
	GetWorldTimerManager().SetTimer(TimerHandle, [this]()
		{
			MeshComponent->DestroyComponent();

			if (DebrisClass)
			{
				for (size_t i = 0; i < DebrisMeshes.Num(); ++i)
				{
					FVector SpawnLocation = GetActorLocation() + DebrisLocationOffsets[i].GetLocation();
					FTransform SpawnTransform;
					SpawnTransform.SetLocation(SpawnLocation);
					AA_Debris* DefferedActor = GetWorld()->SpawnActorDeferred<AA_Debris>(DebrisClass, SpawnTransform);
					DefferedActor->FindComponentByClass<UStaticMeshComponent>()->SetStaticMesh(DebrisMeshes[i]);
					DefferedActor->FinishSpawning(SpawnTransform);
				}
			}

			Destroy();
		}, 10.0f, false);

}

void AA_EnemyBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

