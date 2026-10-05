#include "LocationVisitResetPayload.h"

#include "../LocationSystem/WorldMapAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../LocationSystem/FloorAsset.h"

// ─────────────────────────────────────────────────────────────────────────────
ULocationVisitResetPayload* ULocationVisitResetPayload::SetupAll()
{
    bResetAll = true;
    TargetMap = nullptr;
    TargetRegion = nullptr;
    TargetStreet = nullptr;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;
    return this;
}

// ─────────────────────────────────────────────────────────────────────────────
ULocationVisitResetPayload* ULocationVisitResetPayload::SelectMap(UWorldMapAsset* InMap)
{
    bResetAll = false;
    TargetMap = InMap;
    // Всё глубже карты сбрасываем.
    TargetRegion = nullptr;
    TargetStreet = nullptr;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;
    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectRegion(UWorldRegionAsset* InRegion)
{
    bResetAll = false;
    TargetRegion = InRegion;
    TargetStreet = nullptr;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;

    // Синхронизируем карту по родителю региона, если не задана.
    if (InRegion && !TargetMap)
    {
        TargetMap = InRegion->ParentWorldMap.LoadSynchronous();
    }
    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectStreet(UStreetAsset* InStreet)
{
    bResetAll = false;
    TargetStreet = InStreet;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;

    if (InStreet && !TargetRegion)
    {
        TargetRegion = InStreet->ParentWorldRegion.LoadSynchronous();
    }
    if (InStreet && !TargetMap && TargetRegion)
    {
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();
    }
    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectBuilding(UInteriorSetAsset* InBuilding)
{
    bResetAll = false;
    TargetBuilding = InBuilding;
    TargetFloor = nullptr;

    if (InBuilding && !TargetStreet)
    {
        TargetStreet = InBuilding->ParentStreet.LoadSynchronous();
    }
    if (InBuilding && !TargetRegion && TargetStreet)
    {
        TargetRegion = TargetStreet->ParentWorldRegion.LoadSynchronous();
    }
    if (InBuilding && !TargetMap && TargetRegion)
    {
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();
    }
    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectFloor(UFloorAsset* InFloor)
{
    bResetAll = false;
    TargetFloor = InFloor;

    if (InFloor && !TargetBuilding)
    {
        TargetBuilding = InFloor->ParentInteriorSet.LoadSynchronous();
    }
    if (InFloor && !TargetStreet && TargetBuilding)
    {
        TargetStreet = TargetBuilding->ParentStreet.LoadSynchronous();
    }
    if (InFloor && !TargetRegion && TargetStreet)
    {
        TargetRegion = TargetStreet->ParentWorldRegion.LoadSynchronous();
    }
    if (InFloor && !TargetMap && TargetRegion)
    {
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();
    }
    return this;
}

// ─────────────────────────────────────────────────────────────────────────────
TArray<UWorldRegionAsset*> ULocationVisitResetPayload::GetAvailableRegions() const
{
    TArray<UWorldRegionAsset*> Result;
    if (!TargetMap) return Result;

    for (const TSoftObjectPtr<UWorldRegionAsset>& Ref : TargetMap->Regions)
    {
        if (UWorldRegionAsset* Region = Ref.LoadSynchronous())
            Result.Add(Region);
    }
    return Result;
}

TArray<UStreetAsset*> ULocationVisitResetPayload::GetAvailableStreets() const
{
    TArray<UStreetAsset*> Result;
    if (!TargetRegion) return Result;

    for (const TSoftObjectPtr<UStreetAsset>& Ref : TargetRegion->Streets)
    {
        if (UStreetAsset* Street = Ref.LoadSynchronous())
            Result.Add(Street);
    }
    return Result;
}

TArray<UInteriorSetAsset*> ULocationVisitResetPayload::GetAvailableBuildings() const
{
    TArray<UInteriorSetAsset*> Result;
    if (!TargetStreet) return Result;

    for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : TargetStreet->InteriorSets)
    {
        if (UInteriorSetAsset* Building = Ref.LoadSynchronous())
            Result.Add(Building);
    }
    return Result;
}

TArray<UFloorAsset*> ULocationVisitResetPayload::GetAvailableFloors() const
{
    TArray<UFloorAsset*> Result;
    if (!TargetBuilding) return Result;

    for (const TSoftObjectPtr<UFloorAsset>& Ref : TargetBuilding->Floors)
    {
        if (UFloorAsset* Floor = Ref.LoadSynchronous())
            Result.Add(Floor);
    }
    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
ULocationVisitResetPayload* ULocationVisitResetPayload::Setup(
    UWorldMapAsset* InMap,
    UWorldRegionAsset* InRegion,
    UStreetAsset* InStreet,
    UInteriorSetAsset* InBuilding,
    UFloorAsset* InFloor)
{
    bResetAll = false;
    TargetMap = InMap;
    TargetRegion = InRegion;
    TargetStreet = InStreet;
    TargetBuilding = InBuilding;
    TargetFloor = InFloor;
    return this;
}