#include "BookAndWords/C_Word.h"

AC_Word::AC_Word()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AC_Word::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AC_Word::BeginPlay()
{
	Super::BeginPlay();
}

void AC_Word::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AC_Word::Absorbing()
{
}
