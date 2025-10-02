#include "ArrayEffect/A_QueueEffect.h"

AA_QueueEffect::AA_QueueEffect()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_QueueEffect::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_QueueEffect::BeginPlay()
{
	Super::BeginPlay();
}

void AA_QueueEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_QueueEffect::GetTextCommand(FText Command)
{
	Super::GetTextCommand(Command);
}