#include "LocationEditorUtils.h"

// Editor-only includes должны быть ДО использования этих типов
#if WITH_EDITOR
#include "WorldMapAsset.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"
#include "LocationSpatialTypes.h"
#include "FloorAssignerEditorLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Misc/MessageDialog.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"

namespace FLocationEditorUtilsImpl
{
    static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UInteriorSetAsset>& Ptr, UInteriorSetAsset* Expected)
    {
        if (Ptr.IsNull()) return true;
        UInteriorSetAsset* Loaded = Ptr.Get();
        if (!Loaded) Loaded = Ptr.LoadSynchronous();
        return Loaded != Expected;
    }
    static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UStreetAsset>& Ptr, UStreetAsset* Expected)
    {
        if (Ptr.IsNull()) return true;
        UStreetAsset* Loaded = Ptr.Get();
        if (!Loaded) Loaded = Ptr.LoadSynchronous();
        return Loaded != Expected;
    }
    static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UWorldRegionAsset>& Ptr, UWorldRegionAsset* Expected)
    {
        if (Ptr.IsNull()) return true;
        UWorldRegionAsset* Loaded = Ptr.Get();
        if (!Loaded) Loaded = Ptr.LoadSynchronous();
        return Loaded != Expected;
    }
    static bool SoftPtrNeedsUpdate(const TSoftObjectPtr<UWorldMapAsset>& Ptr, UWorldMapAsset* Expected)
    {
        if (Ptr.IsNull()) return true;
        UWorldMapAsset* Loaded = Ptr.Get();
        if (!Loaded) Loaded = Ptr.LoadSynchronous();
        return Loaded != Expected;
    }

    // Forward declaration
    int32 AutoFillParentsForInteriorSet(UInteriorSetAsset* InteriorSet);

    int32 AutoFillParentsForStreet(UStreetAsset* Street)
    {
        if (!IsValid(Street)) return 0;
        int32 N = 0;
        for (FLocationZone& Z : Street->Zones)
        {
            if (Z.ParentContextType == ELocationContextType::None || !Z.ParentLocationID.IsValid())
            {
                Street->Modify();
                if (Z.ParentContextType == ELocationContextType::None) Z.ParentContextType = ELocationContextType::Street;
                if (!Z.ParentLocationID.IsValid()) Z.ParentLocationID = Street->StreetID;
                Street->MarkPackageDirty(); N++;
            }
        }
        for (FLocationAnchor& A : Street->Anchors)
        {
            if (A.ParentContextType == ELocationContextType::None || !A.ParentLocationID.IsValid())
            {
                Street->Modify();
                if (A.ParentContextType == ELocationContextType::None) A.ParentContextType = ELocationContextType::Street;
                if (!A.ParentLocationID.IsValid()) A.ParentLocationID = Street->StreetID;
                Street->MarkPackageDirty(); N++;
            }
        }
        for (FLocationTransitionPoint& TP : Street->TransitionPoints)
        {
            bool b = false;
            if (!TP.TransitionPointID.IsValid()) { Street->Modify(); TP.TransitionPointID = FGuid::NewGuid(); b = true; }
            if (!TP.SourceLocationID.IsValid()) { Street->Modify(); TP.SourceContextType = ELocationContextType::Street; TP.SourceLocationID = Street->StreetID; b = true; }
            if (b) { Street->MarkPackageDirty(); N++; }
        }
        for (const TSoftObjectPtr<UInteriorSetAsset>& R : Street->InteriorSets)
        {
            if (R.IsNull()) continue;
            UInteriorSetAsset* IS = R.LoadSynchronous();
            if (!IsValid(IS)) continue;
            if (SoftPtrNeedsUpdate(IS->ParentStreet, Street)) { IS->Modify(); IS->ParentStreet = Street; IS->MarkPackageDirty(); N++; }
            N += AutoFillParentsForInteriorSet(IS);
        }
        return N;
    }

    int32 AutoFillParentsForInteriorSet(UInteriorSetAsset* IS)
    {
        if (!IsValid(IS)) return 0;
        int32 N = 0;
        for (FLocationTransitionPoint& TP : IS->EntranceTransitionPoints)
        {
            bool b = false;
            if (!TP.TransitionPointID.IsValid()) { IS->Modify(); TP.TransitionPointID = FGuid::NewGuid(); b = true; }
            if (!TP.DestinationLocationID.IsValid() && !IS->Floors.IsEmpty())
            {
                UFloorAsset* FF = IS->Floors[0].LoadSynchronous();
                if (IsValid(FF)) { IS->Modify(); TP.DestinationContextType = ELocationContextType::Floor; TP.DestinationLocationID = FF->FloorID; b = true; }
            }
            if (!TP.SourceLocationID.IsValid())
            {
                UStreetAsset* S = IS->ParentStreet.LoadSynchronous();
                if (IsValid(S)) { IS->Modify(); TP.SourceContextType = ELocationContextType::Street; TP.SourceLocationID = S->StreetID; b = true; }
            }
            if (b) { IS->MarkPackageDirty(); N++; }
        }
        for (const TSoftObjectPtr<UFloorAsset>& FR : IS->Floors)
        {
            if (FR.IsNull()) continue;
            UFloorAsset* Floor = FR.LoadSynchronous();
            if (!IsValid(Floor)) continue;
            if (SoftPtrNeedsUpdate(Floor->ParentInteriorSet, IS)) { Floor->Modify(); Floor->ParentInteriorSet = IS; Floor->MarkPackageDirty(); N++; }
            for (FLocationZone& Z : Floor->Zones)
            {
                if (Z.ParentContextType == ELocationContextType::None || !Z.ParentLocationID.IsValid())
                {
                    Floor->Modify();
                    if (Z.ParentContextType == ELocationContextType::None) Z.ParentContextType = ELocationContextType::Floor;
                    if (!Z.ParentLocationID.IsValid()) Z.ParentLocationID = Floor->FloorID;
                    Floor->MarkPackageDirty(); N++;
                }
            }
            for (FLocationAnchor& A : Floor->Anchors)
            {
                if (A.ParentContextType == ELocationContextType::None || !A.ParentLocationID.IsValid())
                {
                    Floor->Modify();
                    if (A.ParentContextType == ELocationContextType::None) A.ParentContextType = ELocationContextType::Floor;
                    if (!A.ParentLocationID.IsValid()) A.ParentLocationID = Floor->FloorID;
                    Floor->MarkPackageDirty(); N++;
                }
            }
            for (FLocationTransitionPoint& TP : Floor->TransitionPoints)
            {
                bool b = false;
                if (!TP.TransitionPointID.IsValid()) { Floor->Modify(); TP.TransitionPointID = FGuid::NewGuid(); b = true; }
                if (!TP.SourceLocationID.IsValid()) { Floor->Modify(); TP.SourceContextType = ELocationContextType::Floor; TP.SourceLocationID = Floor->FloorID; b = true; }
                if (b) { Floor->MarkPackageDirty(); N++; }
            }
        }
        return N;
    }

    int32 AutoFillParentsForWorldMap(UWorldMapAsset* WorldMap)
    {
        if (!IsValid(WorldMap))
        {
            if (FModuleManager::Get().IsModuleLoaded("ContentBrowser"))
            {
                FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
                TArray<FAssetData> Sel; CB.Get().GetSelectedAssets(Sel);
                for (const FAssetData& AD : Sel)
                    if (UWorldMapAsset* WM = Cast<UWorldMapAsset>(AD.GetAsset())) { WorldMap = WM; break; }
            }
        }
        if (!IsValid(WorldMap))
        {
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No WorldMap selected.")));
            return 0;
        }
        int32 N = 0;
        for (const TSoftObjectPtr<UWorldRegionAsset>& RR : WorldMap->Regions)
        {
            if (RR.IsNull()) continue;
            UWorldRegionAsset* Region = RR.LoadSynchronous();
            if (!IsValid(Region)) continue;
            if (SoftPtrNeedsUpdate(Region->ParentWorldMap, WorldMap)) { Region->Modify(); Region->ParentWorldMap = WorldMap; Region->MarkPackageDirty(); N++; }
            for (const TSoftObjectPtr<UStreetAsset>& SR : Region->Streets)
            {
                if (SR.IsNull()) continue;
                UStreetAsset* Street = SR.LoadSynchronous();
                if (!IsValid(Street)) continue;
                if (SoftPtrNeedsUpdate(Street->ParentWorldRegion, Region)) { Street->Modify(); Street->ParentWorldRegion = Region; Street->MarkPackageDirty(); N++; }
                N += AutoFillParentsForStreet(Street);
            }
        }
        FMessageDialog::Open(EAppMsgType::Ok, FText::Format(FText::FromString(TEXT("AutoFill complete. Changes: {0}")), FText::AsNumber(N)));
        return N;
    }

    TArray<UWorldMapAsset*> GetAllWorldMapAssets()
    {
        TArray<UWorldMapAsset*> Result;
        FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        TArray<FAssetData> List;
        ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("WorldMapAsset")), List, true);
        for (const FAssetData& AD : List)
        {
            if (UWorldMapAsset* M = Cast<UWorldMapAsset>(AD.GetAsset())) { Result.Add(M); }
            else if (UWorldMapAsset* M2 = Cast<UWorldMapAsset>(AD.ToSoftObjectPath().TryLoad())) { Result.Add(M2); }
        }
        return Result;
    }
} // namespace FLocationEditorUtilsImpl

#endif // WITH_EDITOR

// ---- Реализации методов ULocationEditorUtils — делегируют в impl/FFloorAssignerEditorLibrary ----

int32 ULocationEditorUtils::AutoFillParentsForWorldMap(UWorldMapAsset* WorldMap)
{
#if WITH_EDITOR
    return FLocationEditorUtilsImpl::AutoFillParentsForWorldMap(WorldMap);
#else
    return 0;
#endif
}

int32 ULocationEditorUtils::AutoFillParentsForStreet(UStreetAsset* Street)
{
#if WITH_EDITOR
    return FLocationEditorUtilsImpl::AutoFillParentsForStreet(Street);
#else
    return 0;
#endif
}

int32 ULocationEditorUtils::AutoFillParentsForInteriorSet(UInteriorSetAsset* InteriorSet)
{
#if WITH_EDITOR
    return FLocationEditorUtilsImpl::AutoFillParentsForInteriorSet(InteriorSet);
#else
    return 0;
#endif
}

TArray<UWorldMapAsset*> ULocationEditorUtils::GetAllWorldMapAssets()
{
#if WITH_EDITOR
    return FLocationEditorUtilsImpl::GetAllWorldMapAssets();
#else
    return {};
#endif
}

TMap<FGuid, FText> ULocationEditorUtils::GetWorldMaps()
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::GetWorldMaps();
#else
    return {};
#endif
}

TMap<FGuid, FText> ULocationEditorUtils::GetRegions(const FGuid& WorldMapGuid)
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::GetRegions(WorldMapGuid);
#else
    return {};
#endif
}

TMap<FGuid, FText> ULocationEditorUtils::GetStreets(const FGuid& WorldRegionGuid)
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::GetStreets(WorldRegionGuid);
#else
    return {};
#endif
}

TMap<FGuid, FText> ULocationEditorUtils::GetInteriorSets(const FGuid& StreetGuid)
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::GetInteriorSets(StreetGuid);
#else
    return {};
#endif
}

TMap<FGuid, FText> ULocationEditorUtils::GetFloors(const FGuid& InteriorSetGuid)
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::GetFloors(InteriorSetGuid);
#else
    return {};
#endif
}

int32 ULocationEditorUtils::ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid)
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::ApplyFloorToSelectedActors(FloorGuid, InteriorSetGuid);
#else
    return 0;
#endif
}

int32 ULocationEditorUtils::SelectActorsByFloor(const FGuid& FloorGuid)
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::SelectActorsByFloor(FloorGuid);
#else
    return 0;
#endif
}

int32 ULocationEditorUtils::UnregisterSelectedActors()
{
#if WITH_EDITOR
    return FFloorAssignerEditorLibrary::UnregisterSelectedActors();
#else
    return 0;
#endif
}