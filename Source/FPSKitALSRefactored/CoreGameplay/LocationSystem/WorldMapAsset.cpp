#include "WorldMapAsset.h"

#if WITH_EDITOR
void UWorldMapAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (WorldMapID.IsValid() == false)
    {
        WorldMapID = FGuid::NewGuid();
    }
}
#endif