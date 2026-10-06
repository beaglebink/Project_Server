#include "LocationVisitResetPayload.h"

#include "../LocationSystem/WorldMapAsset.h"
#include "../LocationSystem/WorldRegionAsset.h"
#include "../LocationSystem/StreetAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "../LocationSystem/FloorAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "UObject/SoftObjectPath.h"

namespace
{
    // Универсальное сравнение: сначала по DisplayName, если оно непустое,
    // иначе — по имени ассета. Удобно для ассетов, у которых DisplayName пуст.
    template<typename TAsset>
    bool MatchesDisplayName(const TAsset* Asset, const FText& InDisplayName)
    {
        if (!Asset) return false;

        const FString Target = InDisplayName.ToString();
        if (Target.IsEmpty()) return false;

        if (!Asset->DisplayName.IsEmpty())
            return Asset->DisplayName.ToString().Equals(Target, ESearchCase::CaseSensitive);

        return Asset->GetName().Equals(Target, ESearchCase::CaseSensitive);
    }

    // Global lookup через AssetRegistry. Используется, когда родительский
    // контекст не задан.
    template<typename TAsset>
    TAsset* FindAssetByDisplayName_Global(const FText& InDisplayName, const TCHAR* ClassName)
    {
        FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> AssetDataList;
        ARM.Get().GetAssetsByClass(
            FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), ClassName),
            AssetDataList, true);

        for (const FAssetData& AD : AssetDataList)
        {
            TAsset* Asset = Cast<TAsset>(AD.ToSoftObjectPath().TryLoad());
            if (MatchesDisplayName(Asset, InDisplayName))
                return Asset;
        }
        return nullptr;
    }

    // ── Per-type finders ─────────────────────────────────────────────────

    UWorldMapAsset* FindMapByDisplayName(const FText& InDisplayName)
    {
        return FindAssetByDisplayName_Global<UWorldMapAsset>(InDisplayName, TEXT("WorldMapAsset"));
    }

    UWorldRegionAsset* FindRegionByDisplayName(const FText& InDisplayName, UWorldMapAsset* Scope)
    {
        if (Scope)
        {
            for (const TSoftObjectPtr<UWorldRegionAsset>& Ref : Scope->Regions)
            {
                UWorldRegionAsset* R = Ref.LoadSynchronous();
                if (MatchesDisplayName(R, InDisplayName)) return R;
            }
            return nullptr;
        }
        return FindAssetByDisplayName_Global<UWorldRegionAsset>(InDisplayName, TEXT("WorldRegionAsset"));
    }

    UStreetAsset* FindStreetByDisplayName(const FText& InDisplayName, UWorldRegionAsset* Scope)
    {
        if (Scope)
        {
            for (const TSoftObjectPtr<UStreetAsset>& Ref : Scope->Streets)
            {
                UStreetAsset* S = Ref.LoadSynchronous();
                if (MatchesDisplayName(S, InDisplayName)) return S;
            }
            return nullptr;
        }
        return FindAssetByDisplayName_Global<UStreetAsset>(InDisplayName, TEXT("StreetAsset"));
    }

    UInteriorSetAsset* FindBuildingByDisplayName(const FText& InDisplayName, UStreetAsset* Scope)
    {
        if (Scope)
        {
            for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Scope->InteriorSets)
            {
                UInteriorSetAsset* B = Ref.LoadSynchronous();
                if (MatchesDisplayName(B, InDisplayName)) return B;
            }
            return nullptr;
        }
        return FindAssetByDisplayName_Global<UInteriorSetAsset>(InDisplayName, TEXT("InteriorSetAsset"));
    }

    UFloorAsset* FindFloorByDisplayName(const FText& InDisplayName, UInteriorSetAsset* Scope)
    {
        if (Scope)
        {
            for (const TSoftObjectPtr<UFloorAsset>& Ref : Scope->Floors)
            {
                UFloorAsset* F = Ref.LoadSynchronous();
                if (MatchesDisplayName(F, InDisplayName)) return F;
            }
            return nullptr;
        }
        return FindAssetByDisplayName_Global<UFloorAsset>(InDisplayName, TEXT("FloorAsset"));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SetupAll
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
// Каскадные Select* — без изменений
// ─────────────────────────────────────────────────────────────────────────────
ULocationVisitResetPayload* ULocationVisitResetPayload::SelectMap(UWorldMapAsset* InMap)
{
    bResetAll = false;
    TargetMap = InMap;
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

    if (InRegion && !TargetMap)
        TargetMap = InRegion->ParentWorldMap.LoadSynchronous();

    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectStreet(UStreetAsset* InStreet)
{
    bResetAll = false;
    TargetStreet = InStreet;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;

    if (InStreet && !TargetRegion)
        TargetRegion = InStreet->ParentWorldRegion.LoadSynchronous();
    if (InStreet && !TargetMap && TargetRegion)
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();

    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectBuilding(UInteriorSetAsset* InBuilding)
{
    bResetAll = false;
    TargetBuilding = InBuilding;
    TargetFloor = nullptr;

    if (InBuilding && !TargetStreet)
        TargetStreet = InBuilding->ParentStreet.LoadSynchronous();
    if (InBuilding && !TargetRegion && TargetStreet)
        TargetRegion = TargetStreet->ParentWorldRegion.LoadSynchronous();
    if (InBuilding && !TargetMap && TargetRegion)
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();

    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SelectFloor(UFloorAsset* InFloor)
{
    bResetAll = false;
    TargetFloor = InFloor;

    if (InFloor && !TargetBuilding)
        TargetBuilding = InFloor->ParentInteriorSet.LoadSynchronous();
    if (InFloor && !TargetStreet && TargetBuilding)
        TargetStreet = TargetBuilding->ParentStreet.LoadSynchronous();
    if (InFloor && !TargetRegion && TargetStreet)
        TargetRegion = TargetStreet->ParentWorldRegion.LoadSynchronous();
    if (InFloor && !TargetMap && TargetRegion)
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();

    return this;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetAvailable* — без изменений
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
// Setup* по DisplayName
// ─────────────────────────────────────────────────────────────────────────────

ULocationVisitResetPayload* ULocationVisitResetPayload::SetupMap(const FText InDisplayName)
{
    UWorldMapAsset* Map = FindMapByDisplayName(InDisplayName);
    if (!Map)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationVisitResetPayload::SetupMap: map with DisplayName '%s' not found"),
            *InDisplayName.ToString());
        return this;
    }

    bResetAll = false;
    TargetMap = Map;
    TargetRegion = nullptr;
    TargetStreet = nullptr;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;
    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SetupRegion(const FText InDisplayName)
{
    // Ищем в текущем TargetMap, если он задан; иначе по всем ассетам.
    UWorldRegionAsset* Region = FindRegionByDisplayName(InDisplayName, TargetMap);
    if (!Region)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationVisitResetPayload::SetupRegion: region with DisplayName '%s' not found%s"),
            *InDisplayName.ToString(),
            TargetMap ? TEXT(" in current TargetMap") : TEXT(""));
        return this;
    }

    bResetAll = false;
    TargetRegion = Region;
    TargetStreet = nullptr;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;

    // Подтягиваем родительскую карту, если её ещё нет.
    if (!TargetMap)
        TargetMap = Region->ParentWorldMap.LoadSynchronous();

    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SetupStreet(const FText InDisplayName)
{
    UStreetAsset* Street = FindStreetByDisplayName(InDisplayName, TargetRegion);
    if (!Street)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationVisitResetPayload::SetupStreet: street with DisplayName '%s' not found%s"),
            *InDisplayName.ToString(),
            TargetRegion ? TEXT(" in current TargetRegion") : TEXT(""));
        return this;
    }

    bResetAll = false;
    TargetStreet = Street;
    TargetBuilding = nullptr;
    TargetFloor = nullptr;

    if (!TargetRegion)
        TargetRegion = Street->ParentWorldRegion.LoadSynchronous();
    if (!TargetMap && TargetRegion)
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();

    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SetupBuilding(const FText InDisplayName)
{
    UInteriorSetAsset* Building = FindBuildingByDisplayName(InDisplayName, TargetStreet);
    if (!Building)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationVisitResetPayload::SetupBuilding: building with DisplayName '%s' not found%s"),
            *InDisplayName.ToString(),
            TargetStreet ? TEXT(" in current TargetStreet") : TEXT(""));
        return this;
    }

    bResetAll = false;
    TargetBuilding = Building;
    TargetFloor = nullptr;

    if (!TargetStreet)
        TargetStreet = Building->ParentStreet.LoadSynchronous();
    if (!TargetRegion && TargetStreet)
        TargetRegion = TargetStreet->ParentWorldRegion.LoadSynchronous();
    if (!TargetMap && TargetRegion)
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();

    return this;
}

ULocationVisitResetPayload* ULocationVisitResetPayload::SetupFloor(const FText InDisplayName)
{
    UFloorAsset* Floor = FindFloorByDisplayName(InDisplayName, TargetBuilding);
    if (!Floor)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("LocationVisitResetPayload::SetupFloor: floor with DisplayName '%s' not found%s"),
            *InDisplayName.ToString(),
            TargetBuilding ? TEXT(" in current TargetBuilding") : TEXT(""));
        return this;
    }

    bResetAll = false;
    TargetFloor = Floor;

    if (!TargetBuilding)
        TargetBuilding = Floor->ParentInteriorSet.LoadSynchronous();
    if (!TargetStreet && TargetBuilding)
        TargetStreet = TargetBuilding->ParentStreet.LoadSynchronous();
    if (!TargetRegion && TargetStreet)
        TargetRegion = TargetStreet->ParentWorldRegion.LoadSynchronous();
    if (!TargetMap && TargetRegion)
        TargetMap = TargetRegion->ParentWorldMap.LoadSynchronous();

    return this;
}