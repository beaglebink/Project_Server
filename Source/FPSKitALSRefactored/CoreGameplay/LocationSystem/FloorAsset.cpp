#include "FloorAsset.h"

#if WITH_EDITOR
void UFloorAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (FloorID.IsValid() == false)
    {
        FloorID = FGuid::NewGuid();
    }
}
#endif