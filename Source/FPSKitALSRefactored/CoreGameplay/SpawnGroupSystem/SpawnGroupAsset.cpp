# include "SpawnGroupAsset.h"

#if WITH_EDITOR
void USpawnGroupAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (!GroupId.IsValid())
    {
        GroupId = FGuid::NewGuid();
    }
}

void USpawnGroupAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    // При дублировании ассета генерируем новый GUID
    //if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(USpawnGroupAsset, GroupId))
    //{
        if (!GroupId.IsValid())
        {
            GroupId = FGuid::NewGuid();
        }
    //}
}
#endif