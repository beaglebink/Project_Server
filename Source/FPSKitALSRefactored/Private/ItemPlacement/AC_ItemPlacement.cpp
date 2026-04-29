#include "ItemPlacement/AC_ItemPlacement.h"

UAC_ItemPlacement::UAC_ItemPlacement()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAC_ItemPlacement::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAC_ItemPlacement::BeginPlay()
{
	Super::BeginPlay();
}
