#include "GM_Base.h"
#include "A_EnemyManager.h"

AGM_Base::AGM_Base()
{
}

void AGM_Base::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->SpawnActor<AA_EnemyManager>();
}

void AGM_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
