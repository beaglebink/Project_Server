#include "StreetAsset.h"

#if WITH_EDITOR
void UStreetAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (StreetID.IsValid() == false)
    {
        StreetID = FGuid::NewGuid();
    }
}
#endif