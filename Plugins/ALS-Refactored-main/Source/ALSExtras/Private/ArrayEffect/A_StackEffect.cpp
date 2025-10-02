#include "ArrayEffect/A_StackEffect.h"

AA_StackEffect::AA_StackEffect()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AA_StackEffect::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AA_StackEffect::BeginPlay()
{
	Super::BeginPlay();
}

void AA_StackEffect::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AA_StackEffect::GetTextCommand(FText Command)
{
	Super::GetTextCommand(Command);
}