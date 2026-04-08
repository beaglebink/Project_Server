#include "WorldRegionAsset.h"

#if WITH_EDITOR
void UWorldRegionAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (WorldRegionID.IsValid() == false)
    {
        WorldRegionID = FGuid::NewGuid();
    }
}
#endif