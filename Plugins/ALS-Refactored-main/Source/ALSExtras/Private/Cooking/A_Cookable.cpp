#include "Cooking/A_Cookable.h"

AA_Cookable::AA_Cookable()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AA_Cookable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

}

void AA_Cookable::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AA_Cookable::BeginPlay()
{
	Super::BeginPlay();

}

void AA_Cookable::Destroyed()
{
	Super::Destroyed();

}

void AA_Cookable::HandleCutting_Implementation(UPARAM(ref)FHitResult& Hit, FRotator WeaponRotation)
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Cookable was cut!")));
}
