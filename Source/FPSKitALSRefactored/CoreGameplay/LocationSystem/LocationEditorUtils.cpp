#include "LocationEditorUtils.h"

#if WITH_EDITOR

#include "WorldMapAsset.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"
#include "LocationSpatialTypes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Misc/ScopedSlowTask.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Misc/MessageDialog.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

// Helpers: проверяет что SoftPtr либо null либо указывает не на тот объект
static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UInteriorSetAsset>& Ptr, UInteriorSetAsset* Expected)
{
    if (Ptr.IsNull()) return true;
    UInteriorSetAsset* Loaded = Ptr.Get(); // не грузит, только проверяет в памяти
    if (Loaded == nullptr) Loaded = Ptr.LoadSynchronous();
    return Loaded != Expected;
}

static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UStreetAsset>& Ptr, UStreetAsset* Expected)
{
    if (Ptr.IsNull()) return true;
    UStreetAsset* Loaded = Ptr.Get();
    if (Loaded == nullptr) Loaded = Ptr.LoadSynchronous();
    return Loaded != Expected;
}

static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UWorldRegionAsset>& Ptr, UWorldRegionAsset* Expected)
{
    if (Ptr.IsNull()) return true;
    UWorldRegionAsset* Loaded = Ptr.Get();
    if (Loaded == nullptr) Loaded = Ptr.LoadSynchronous();
    return Loaded != Expected;
}

static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UWorldMapAsset>& Ptr, UWorldMapAsset* Expected)
{
    if (Ptr.IsNull()) return true;
    UWorldMapAsset* Loaded = Ptr.Get();
    if (Loaded == nullptr) Loaded = Ptr.LoadSynchronous();
    return Loaded != Expected;
}

#endif // WITH_EDITOR

int32 ULocationEditorUtils::AutoFillParentsForWorldMap(UWorldMapAsset* WorldMap)
{
#if WITH_EDITOR
    // If caller didn't provide WorldMap, try to get first selected WorldMap asset from Content Browser
    if (!IsValid(WorldMap))
    {
        if (FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
        {
            FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
            TArray<FAssetData> SelectedAssets;
            CBModule.Get().GetSelectedAssets(SelectedAssets);

            for (const FAssetData& AssetData : SelectedAssets)
            {
                UObject* Obj = AssetData.GetAsset();
                if (UWorldMapAsset* WM = Cast<UWorldMapAsset>(Obj))
                {
                    WorldMap = WM;
                    break;
                }
            }
        }
    }

    if (!IsValid(WorldMap))
    {
        FText Msg = FText::FromString(TEXT("No WorldMap asset selected.\n\nPlease select a WorldMap asset in the Content Browser and try again."));
        FMessageDialog::Open(EAppMsgType::Ok, Msg);
        UE_LOG(LogTemp, Warning, TEXT("AutoFillParentsForWorldMap: WorldMap == null"));
        return 0;
    }

    // Local statistics
    struct FAutoFillStats
    {
        int32 Regions = 0;
        int32 Streets = 0;
        int32 Interiors = 0;
        int32 Floors = 0;
        int32 Zones = 0;
        int32 Anchors = 0;
        int32 TransitionPoints = 0;
        int32 GUIDs = 0;
        int32 TotalChanges = 0;

        void AddChange(int32 Count = 1) { TotalChanges += Count; }
    };

    FAutoFillStats Stats;

    auto AutoFillFloor_Internal = [&](UFloorAsset* Floor, UInteriorSetAsset* InteriorSet) -> int32
    {
        if (!IsValid(Floor) || !IsValid(InteriorSet)) return 0;
        int32 LocalModified = 0;

        // Ключевое исправление: проверяем через Get()/LoadSynchronous, а не IsNull()
        if (SoftPtrNeedsUpdate(Floor->ParentInteriorSet, InteriorSet))
        {
            Floor->Modify();
            Floor->ParentInteriorSet = InteriorSet; // TSoftObjectPtr принимает raw pointer напрямую
            Floor->MarkPackageDirty();
            LocalModified++;
            Stats.Floors++;
            Stats.AddChange();
            UE_LOG(LogTemp, Log, TEXT("Set ParentInteriorSet for Floor '%s' -> '%s'"),
                *Floor->GetName(), *InteriorSet->GetName());
        }

        // Zones on floor
        for (FLocationZone& Zone : Floor->Zones)
        {
            if (Zone.ParentContextType == ELocationContextType::None || !Zone.ParentLocationID.IsValid())
            {
                Floor->Modify();
                if (Zone.ParentContextType == ELocationContextType::None) Zone.ParentContextType = ELocationContextType::Floor;
                if (!Zone.ParentLocationID.IsValid()) Zone.ParentLocationID = Floor->FloorID;
                Floor->MarkPackageDirty();
                LocalModified++;
                Stats.Zones++;
                Stats.AddChange();
            }
        }

        // Anchors on floor
        for (FLocationAnchor& Anchor : Floor->Anchors)
        {
            if (Anchor.ParentContextType == ELocationContextType::None || !Anchor.ParentLocationID.IsValid())
            {
                Floor->Modify();
                if (Anchor.ParentContextType == ELocationContextType::None) Anchor.ParentContextType = ELocationContextType::Floor;
                if (!Anchor.ParentLocationID.IsValid()) Anchor.ParentLocationID = Floor->FloorID;
                Floor->MarkPackageDirty();
                LocalModified++;
                Stats.Anchors++;
                Stats.AddChange();
            }
        }

        // TransitionPoints on floor
        for (FLocationTransitionPoint& TP : Floor->TransitionPoints)
        {
            bool bChanged = false;
            if (!TP.TransitionPointID.IsValid())
            {
                Floor->Modify();
                TP.TransitionPointID = FGuid::NewGuid();
                bChanged = true;
                Stats.GUIDs++;
            }
            if (!TP.SourceLocationID.IsValid())
            {
                Floor->Modify();
                TP.SourceContextType = ELocationContextType::Floor;
                TP.SourceLocationID = Floor->FloorID;
                bChanged = true;
            }
            if (bChanged)
            {
                Floor->MarkPackageDirty();
                LocalModified++;
                Stats.TransitionPoints++;
                Stats.AddChange();
            }
        }

        return LocalModified;
    };

    auto AutoFillParentsForInteriorSet_Internal = [&](UInteriorSetAsset* InteriorSet, UStreetAsset* Street) -> int32
    {
        if (!IsValid(InteriorSet)) return 0;
        int32 LocalModified = 0;

        // Установка ParentStreet
        if (IsValid(Street) && SoftPtrNeedsUpdate(InteriorSet->ParentStreet, Street))
        {
            InteriorSet->Modify();
            InteriorSet->ParentStreet = Street;
            InteriorSet->MarkPackageDirty();
            LocalModified++;
            Stats.Interiors++;
            Stats.AddChange();
            UE_LOG(LogTemp, Log, TEXT("Set ParentStreet for InteriorSet '%s' -> '%s'"),
                *InteriorSet->GetName(), *Street->GetName());
        }

        // EntranceTransitionPoints
        for (FLocationTransitionPoint& TP : InteriorSet->EntranceTransitionPoints)
        {
            bool bChanged = false;
            if (!TP.TransitionPointID.IsValid())
            {
                InteriorSet->Modify();
                TP.TransitionPointID = FGuid::NewGuid();
                bChanged = true;
                Stats.GUIDs++;
            }

            if (!TP.DestinationLocationID.IsValid())
            {
                if (!InteriorSet->Floors.IsEmpty())
                {
                    UFloorAsset* FirstFloor = InteriorSet->Floors[0].LoadSynchronous();
                    if (IsValid(FirstFloor))
                    {
                        InteriorSet->Modify();
                        TP.DestinationContextType = ELocationContextType::Floor;
                        TP.DestinationLocationID = FirstFloor->FloorID;
                        bChanged = true;
                    }
                }
            }

            if (!TP.SourceLocationID.IsValid())
            {
                UStreetAsset* ParentStreet = InteriorSet->ParentStreet.LoadSynchronous();
                if (IsValid(ParentStreet))
                {
                    InteriorSet->Modify();
                    TP.SourceContextType = ELocationContextType::Street;
                    TP.SourceLocationID = ParentStreet->StreetID;
                    bChanged = true;
                }
            }

            if (bChanged)
            {
                InteriorSet->MarkPackageDirty();
                LocalModified++;
                Stats.TransitionPoints++;
                Stats.AddChange();
            }
        }

        // Floors — передаём InteriorSet явно в AutoFillFloor_Internal
        for (const TSoftObjectPtr<UFloorAsset>& FloorRef : InteriorSet->Floors)
        {
            if (FloorRef.IsNull()) continue;
            UFloorAsset* Floor = FloorRef.LoadSynchronous();
            if (!IsValid(Floor)) continue;

            LocalModified += AutoFillFloor_Internal(Floor, InteriorSet);
        }

        return LocalModified;
    };

    auto AutoFillParentsForStreet_Internal = [&](UStreetAsset* Street, UWorldRegionAsset* Region) -> int32
    {
        if (!IsValid(Street)) return 0;
        int32 LocalModified = 0;

        // Установка ParentWorldRegion
        if (IsValid(Region) && SoftPtrNeedsUpdate(Street->ParentWorldRegion, Region))
        {
            Street->Modify();
            Street->ParentWorldRegion = Region;
            Street->MarkPackageDirty();
            LocalModified++;
            Stats.Streets++;
            Stats.AddChange();
            UE_LOG(LogTemp, Log, TEXT("Set ParentWorldRegion for Street '%s' -> '%s'"),
                *Street->GetName(), *Region->GetName());
        }

        // Zones on street
        for (FLocationZone& Zone : Street->Zones)
        {
            if (Zone.ParentContextType == ELocationContextType::None || !Zone.ParentLocationID.IsValid())
            {
                Street->Modify();
                if (Zone.ParentContextType == ELocationContextType::None) Zone.ParentContextType = ELocationContextType::Street;
                if (!Zone.ParentLocationID.IsValid()) Zone.ParentLocationID = Street->StreetID;
                Street->MarkPackageDirty();
                LocalModified++;
                Stats.Zones++;
                Stats.AddChange();
            }
        }

        // Anchors on street
        for (FLocationAnchor& Anchor : Street->Anchors)
        {
            if (Anchor.ParentContextType == ELocationContextType::None || !Anchor.ParentLocationID.IsValid())
            {
                Street->Modify();
                if (Anchor.ParentContextType == ELocationContextType::None) Anchor.ParentContextType = ELocationContextType::Street;
                if (!Anchor.ParentLocationID.IsValid()) Anchor.ParentLocationID = Street->StreetID;
                Street->MarkPackageDirty();
                LocalModified++;
                Stats.Anchors++;
                Stats.AddChange();
            }
        }

        // TransitionPoints on street
        for (FLocationTransitionPoint& TP : Street->TransitionPoints)
        {
            bool bChanged = false;
            if (!TP.TransitionPointID.IsValid())
            {
                Street->Modify();
                TP.TransitionPointID = FGuid::NewGuid();
                bChanged = true;
                Stats.GUIDs++;
            }
            if (!TP.SourceLocationID.IsValid())
            {
                Street->Modify();
                TP.SourceContextType = ELocationContextType::Street;
                TP.SourceLocationID = Street->StreetID;
                bChanged = true;
            }
            if (bChanged)
            {
                Street->MarkPackageDirty();
                LocalModified++;
                Stats.TransitionPoints++;
                Stats.AddChange();
            }
        }

        // InteriorSets on street — передаём Street явно
        for (const TSoftObjectPtr<UInteriorSetAsset>& IntRef : Street->InteriorSets)
        {
            if (IntRef.IsNull()) continue;
            UInteriorSetAsset* Interior = IntRef.LoadSynchronous();
            if (!IsValid(Interior)) continue;

            LocalModified += AutoFillParentsForInteriorSet_Internal(Interior, Street);
        }

        return LocalModified;
    };

    int32 ModifiedCount = 0;

    for (const TSoftObjectPtr<UWorldRegionAsset>& RegionRef : WorldMap->Regions)
    {
        if (RegionRef.IsNull()) continue;

        UWorldRegionAsset* Region = RegionRef.LoadSynchronous();
        if (!IsValid(Region)) continue;

        if (SoftPtrNeedsUpdate(Region->ParentWorldMap, WorldMap))
        {
            Region->Modify();
            Region->ParentWorldMap = WorldMap;
            Region->MarkPackageDirty();
            ModifiedCount++;
            Stats.Regions++;
            Stats.AddChange();
            UE_LOG(LogTemp, Log, TEXT("Set ParentWorldMap for Region '%s'"), *Region->GetName());
        }

        for (const TSoftObjectPtr<UStreetAsset>& StreetRef : Region->Streets)
        {
            if (StreetRef.IsNull()) continue;
            UStreetAsset* Street = StreetRef.LoadSynchronous();
            if (!IsValid(Street)) continue;

            // Передаём Region явно в лямбду
            int32 StreetMods = AutoFillParentsForStreet_Internal(Street, Region);
            ModifiedCount += StreetMods;
        }
    }

    // Summary dialog
    {
        FString Summary = FString::Printf(
            TEXT("AutoFillParentsForWorldMap completed.\n\nModified assets:\n- WorldRegions: %d\n- Streets: %d\n- InteriorSets: %d\n- Floors: %d\n- Zones: %d\n- Anchors: %d\n- TransitionPoints (structures): %d\n- Generated GUIDs: %d\n\nTotal changes: %d\n\nPlease save modified packages in the Content Browser if you want to keep the changes."),
            Stats.Regions, Stats.Streets, Stats.Interiors, Stats.Floors, Stats.Zones, Stats.Anchors, Stats.TransitionPoints, Stats.GUIDs, Stats.TotalChanges);

        FText Msg = FText::FromString(Summary);
        FMessageDialog::Open(EAppMsgType::Ok, Msg);

        UE_LOG(LogTemp, Log, TEXT("%s"), *Summary);
    }

    return ModifiedCount;
#else
    UE_LOG(LogTemp, Warning, TEXT("AutoFillParentsForWorldMap: editor-only"));
    return 0;
#endif
}

int32 ULocationEditorUtils::AutoFillParentsForStreet(UStreetAsset* Street)
{
#if WITH_EDITOR
    if (!IsValid(Street))
    {
        UE_LOG(LogTemp, Warning, TEXT("AutoFillParentsForStreet: Street == null"));
        return 0;
    }

    int32 ModifiedCount = 0;

    for (FLocationZone& Zone : Street->Zones)
    {
        if (Zone.ParentContextType == ELocationContextType::None || !Zone.ParentLocationID.IsValid())
        {
            Street->Modify();
            if (Zone.ParentContextType == ELocationContextType::None) Zone.ParentContextType = ELocationContextType::Street;
            if (!Zone.ParentLocationID.IsValid()) Zone.ParentLocationID = Street->StreetID;
            Street->MarkPackageDirty();
            ModifiedCount++;
        }
    }

    for (FLocationAnchor& Anchor : Street->Anchors)
    {
        if (Anchor.ParentContextType == ELocationContextType::None || !Anchor.ParentLocationID.IsValid())
        {
            Street->Modify();
            if (Anchor.ParentContextType == ELocationContextType::None) Anchor.ParentContextType = ELocationContextType::Street;
            if (!Anchor.ParentLocationID.IsValid()) Anchor.ParentLocationID = Street->StreetID;
            Street->MarkPackageDirty();
            ModifiedCount++;
        }
    }

    for (FLocationTransitionPoint& TP : Street->TransitionPoints)
    {
        bool bChanged = false;
        if (!TP.TransitionPointID.IsValid())
        {
            Street->Modify();
            TP.TransitionPointID = FGuid::NewGuid();
            bChanged = true;
        }
        if (!TP.SourceLocationID.IsValid())
        {
            Street->Modify();
            TP.SourceContextType = ELocationContextType::Street;
            TP.SourceLocationID = Street->StreetID;
            bChanged = true;
        }
        if (bChanged)
        {
            Street->MarkPackageDirty();
            ModifiedCount++;
        }
    }

    for (const TSoftObjectPtr<UInteriorSetAsset>& IntRef : Street->InteriorSets)
    {
        if (IntRef.IsNull()) continue;
        UInteriorSetAsset* Interior = IntRef.LoadSynchronous();
        if (!IsValid(Interior)) continue;

        if (SoftPtrNeedsUpdate(Interior->ParentStreet, Street))
        {
            Interior->Modify();
            Interior->ParentStreet = Street;
            Interior->MarkPackageDirty();
            ModifiedCount++;
            UE_LOG(LogTemp, Log, TEXT("Set ParentStreet for InteriorSet '%s'"), *Interior->GetName());
        }

        ModifiedCount += AutoFillParentsForInteriorSet(Interior);
    }

    return ModifiedCount;
#else
    UE_LOG(LogTemp, Warning, TEXT("AutoFillParentsForStreet: editor-only"));
    return 0;
#endif
}

int32 ULocationEditorUtils::AutoFillParentsForInteriorSet(UInteriorSetAsset* InteriorSet)
{
#if WITH_EDITOR
    if (!IsValid(InteriorSet))
    {
        UE_LOG(LogTemp, Warning, TEXT("AutoFillParentsForInteriorSet: InteriorSet == null"));
        return 0;
    }

    int32 ModifiedCount = 0;

    for (FLocationTransitionPoint& TP : InteriorSet->EntranceTransitionPoints)
    {
        bool bChanged = false;
        if (!TP.TransitionPointID.IsValid())
        {
            InteriorSet->Modify();
            TP.TransitionPointID = FGuid::NewGuid();
            bChanged = true;
        }

        if (!TP.DestinationLocationID.IsValid())
        {
            if (!InteriorSet->Floors.IsEmpty())
            {
                UFloorAsset* FirstFloor = InteriorSet->Floors[0].LoadSynchronous();
                if (IsValid(FirstFloor))
                {
                    InteriorSet->Modify();
                    TP.DestinationContextType = ELocationContextType::Floor;
                    TP.DestinationLocationID = FirstFloor->FloorID;
                    bChanged = true;
                }
            }
        }

        if (!TP.SourceLocationID.IsValid())
        {
            UStreetAsset* Street = InteriorSet->ParentStreet.LoadSynchronous();
            if (IsValid(Street))
            {
                InteriorSet->Modify();
                TP.SourceContextType = ELocationContextType::Street;
                TP.SourceLocationID = Street->StreetID;
                bChanged = true;
            }
        }

        if (bChanged)
        {
            InteriorSet->MarkPackageDirty();
            ModifiedCount++;
        }
    }

    for (const TSoftObjectPtr<UFloorAsset>& FloorRef : InteriorSet->Floors)
    {
        if (FloorRef.IsNull()) continue;
        UFloorAsset* Floor = FloorRef.LoadSynchronous();
        if (!IsValid(Floor)) continue;

        // Ключевое исправление: SoftPtrNeedsUpdate вместо IsNull()
        if (SoftPtrNeedsUpdate(Floor->ParentInteriorSet, InteriorSet))
        {
            Floor->Modify();
            Floor->ParentInteriorSet = InteriorSet;
            Floor->MarkPackageDirty();
            ModifiedCount++;
            UE_LOG(LogTemp, Log, TEXT("Set ParentInteriorSet for Floor '%s' -> '%s' (public)"),
                *Floor->GetName(), *InteriorSet->GetName());
        }

        for (FLocationZone& Zone : Floor->Zones)
        {
            if (Zone.ParentContextType == ELocationContextType::None || !Zone.ParentLocationID.IsValid())
            {
                Floor->Modify();
                if (Zone.ParentContextType == ELocationContextType::None) Zone.ParentContextType = ELocationContextType::Floor;
                if (!Zone.ParentLocationID.IsValid()) Zone.ParentLocationID = Floor->FloorID;
                Floor->MarkPackageDirty();
                ModifiedCount++;
            }
        }

        for (FLocationAnchor& Anchor : Floor->Anchors)
        {
            if (Anchor.ParentContextType == ELocationContextType::None || !Anchor.ParentLocationID.IsValid())
            {
                Floor->Modify();
                if (Anchor.ParentContextType == ELocationContextType::None) Anchor.ParentContextType = ELocationContextType::Floor;
                if (!Anchor.ParentLocationID.IsValid()) Anchor.ParentLocationID = Floor->FloorID;
                Floor->MarkPackageDirty();
                ModifiedCount++;
            }
        }

        for (FLocationTransitionPoint& TP : Floor->TransitionPoints)
        {
            bool bChanged = false;
            if (!TP.TransitionPointID.IsValid())
            {
                Floor->Modify();
                TP.TransitionPointID = FGuid::NewGuid();
                bChanged = true;
            }
            if (!TP.SourceLocationID.IsValid())
            {
                Floor->Modify();
                TP.SourceContextType = ELocationContextType::Floor;
                TP.SourceLocationID = Floor->FloorID;
                bChanged = true;
            }
            if (bChanged)
            {
                Floor->MarkPackageDirty();
                ModifiedCount++;
            }
        }
    }

    return ModifiedCount;
#else
    UE_LOG(LogTemp, Warning, TEXT("AutoFillParentsForInteriorSet: editor-only"));
    return 0;
#endif
}

// Editor-only helper: получить все UWorldMapAsset в проекте
TArray<UWorldMapAsset*> GetAllWorldMapAssets()
{
    TArray<UWorldMapAsset*> Result;

#if WITH_EDITOR
    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> AssetDataList;

    // UE5: использовать FTopLevelAssetPath с модулем/классом ассета
    ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("WorldMapAsset")), AssetDataList, true);

    for (const FAssetData& AD : AssetDataList)
    {
        // Синхронно получить UObject (загрузит ассет в память, если ещё не загружен)
        UObject* Obj = AD.GetAsset();
        if (UWorldMapAsset* Map = Cast<UWorldMapAsset>(Obj))
        {
            Result.Add(Map);
        }
        else
        {
            // безопасная альтернатива через SoftObjectPath (явная загрузка)
            FSoftObjectPath SoftPath = AD.ToSoftObjectPath();
            if (UWorldMapAsset* Map2 = Cast<UWorldMapAsset>(SoftPath.TryLoad()))
            {
                Result.Add(Map2);
            }
        }
    }
#endif

    return Result;
}