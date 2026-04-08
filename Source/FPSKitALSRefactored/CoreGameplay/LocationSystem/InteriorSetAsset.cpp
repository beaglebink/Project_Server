#include "InteriorSetAsset.h"

#if WITH_EDITOR
void UInteriorSetAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (InteriorSetID.IsValid() == false)
    {
        InteriorSetID = FGuid::NewGuid();
    }
}
#endif