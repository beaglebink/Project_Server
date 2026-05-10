#include "FloorAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"   // === ИЗМЕНЕНИЕ: добавлен include

// === ИЗМЕНЕНИЕ: реализация PostLoad для автоматического заполнения InteriorSetID
void UFloorAsset::PostLoad()
{
    Super::PostLoad();
    // Если GUID ещё не задан, но родительский ассет доступен — извлекаем GUID
    if (!InteriorSetID.IsValid() && ParentInteriorSet.IsValid())
    {
        if (UInteriorSetAsset* LoadedSet = ParentInteriorSet.LoadSynchronous())
        {
            InteriorSetID = LoadedSet->InteriorSetID;
        }
    }
}
// === КОНЕЦ ИЗМЕНЕНИЯ ===

#if WITH_EDITOR
void UFloorAsset::PostInitProperties()
{
    Super::PostInitProperties();
    if (FloorID.IsValid() == false)
    {
        FloorID = FGuid::NewGuid();
    }

    // === ИЗМЕНЕНИЕ: заполняем InteriorSetID при создании ассета, если уже указан ParentInteriorSet
    if (!InteriorSetID.IsValid() && ParentInteriorSet.IsValid())
    {
        if (UInteriorSetAsset* LoadedSet = ParentInteriorSet.LoadSynchronous())
        {
            InteriorSetID = LoadedSet->InteriorSetID;
        }
    }
    // === КОНЕЦ ИЗМЕНЕНИЯ ===
}

// === ИЗМЕНЕНИЕ: автоматическое обновление кэша при смене родительского ассета в редакторе
void UFloorAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UFloorAsset, ParentInteriorSet))
    {
        if (ParentInteriorSet.IsValid())
        {
            InteriorSetID = ParentInteriorSet->InteriorSetID;
        }
        else
        {
            InteriorSetID.Invalidate();
        }
    }
}
// === КОНЕЦ ИЗМЕНЕНИЯ ===
#endif