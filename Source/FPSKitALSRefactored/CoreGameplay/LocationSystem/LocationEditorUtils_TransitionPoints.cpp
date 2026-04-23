#include "LocationEditorUtils.h"

#if WITH_EDITOR

#include "LocationAnchorActor.h"
#include "FloorAsset.h"
#include "InteriorSetAsset.h"
#include "StreetAsset.h"
#include "WorldRegionAsset.h"
#include "WorldMapAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/World.h"
#include "UObject/Package.h"
#include "Modules/ModuleManager.h"

/** Проверяет есть ли якорь с AnchorID на уровне, заданном LevelSoftPath */
static bool IsAnchorPresentOnLevel(const FGuid& AnchorID, const FSoftObjectPath& LevelSoftPath)
{
    if (!AnchorID.IsValid() || !LevelSoftPath.IsValid()) return false;

    const FString PackageName = LevelSoftPath.GetLongPackageName();
    if (PackageName.IsEmpty()) return false;

    UPackage* Pkg = FindPackage(nullptr, *PackageName);
    if (!Pkg)
        Pkg = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Pkg) return false;

    UWorld* World = UWorld::FindWorldInPackage(Pkg);
    if (!World || !World->PersistentLevel) return false;

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        if (ALocationAnchorActor* Anchor = Cast<ALocationAnchorActor>(Actor))
        {
            if (Anchor->AnchorID == AnchorID) return true;
        }
    }
    return false;
}

/** Локальная реализация: находит актор-якорь по AnchorID на уровне LevelSoftPath */
static ALocationAnchorActor* FindAnchorOnLevel_Local(const FGuid& AnchorID, const FSoftObjectPath& LevelSoftPath)
{
    if (!AnchorID.IsValid() || !LevelSoftPath.IsValid()) return nullptr;

    const FString PackageName = LevelSoftPath.GetLongPackageName();
    if (PackageName.IsEmpty()) return nullptr;

    UPackage* Pkg = FindPackage(nullptr, *PackageName);
    if (!Pkg)
        Pkg = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Pkg) return nullptr;

    UWorld* World = UWorld::FindWorldInPackage(Pkg);
    if (!World || !World->PersistentLevel) return nullptr;

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        if (ALocationAnchorActor* Anchor = Cast<ALocationAnchorActor>(Actor))
        {
            if (Anchor->AnchorID == AnchorID) return Anchor;
        }
    }
    return nullptr;
}

bool ULocationEditorUtils::RegisterTransitionPoint(ALocationAnchorActor* SourceAnchor)
{
    if (!IsValid(SourceAnchor) || !SourceAnchor->AnchorID.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("RegisterTransitionPoint: invalid SourceAnchor"));
        return false;
    }

    // Построим запись TP из актора
    FLocationTransitionPoint TP;
    TP.TransitionPointID       = SourceAnchor->AnchorID;
    TP.DisplayName             = SourceAnchor->DisplayName.IsEmpty() ? FText::FromString(SourceAnchor->GetName()) : SourceAnchor->DisplayName;
    TP.TransitionType          = SourceAnchor->TransitionType;
    TP.SourceAnchor            = TSoftObjectPtr<UObject>(SourceAnchor);
    TP.SourceWorldPosition     = SourceAnchor->GetActorLocation();
    TP.SourceWorldOrientation  = SourceAnchor->GetActorRotation();
    TP.DestinationLink         = SourceAnchor->DestinationLink;

    // Попробуем найти актор назначения в Level
    if (TP.DestinationLink.TargetAnchorID.IsValid())
    {
        FSoftObjectPath DestLevelPath;
        if (!TP.DestinationLink.TargetFloor.IsNull())
        {
            if (UFloorAsset* F = TP.DestinationLink.TargetFloor.LoadSynchronous())
                if (!F->FloorLevel.IsNull())
                    DestLevelPath = F->FloorLevel.ToSoftObjectPath();
        }
        if (!DestLevelPath.IsValid() && !TP.DestinationLink.TargetRegion.IsNull())
        {
            if (UWorldRegionAsset* R = TP.DestinationLink.TargetRegion.LoadSynchronous())
                if (!R->RegionLevel.IsNull())
                    DestLevelPath = R->RegionLevel.ToSoftObjectPath();
        }

        if (DestLevelPath.IsValid())
        {
            if (ALocationAnchorActor* DestA = FindAnchorOnLevel_Local(TP.DestinationLink.TargetAnchorID, DestLevelPath))
            {
                TP.DestinationAnchor = TSoftObjectPtr<UObject>(DestA);
                TP.DestinationLink.TargetAnchorActor = TSoftObjectPtr<UObject>(DestA);
            }
        }
    }

    // Определяем ассет регистрации
    UObject* RegAsset = SourceAnchor->GetOwnerRegistrationAsset();
    if (!RegAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("RegisterTransitionPoint: GetOwnerRegistrationAsset returned nullptr for actor '%s'"), *SourceAnchor->GetName());
        return false;
    }

    // Floor
    if (UFloorAsset* Floor = Cast<UFloorAsset>(RegAsset))
    {
        Floor->Modify();
        FLocationTransitionPoint* Existing = Floor->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing) { *Existing = TP; } else { Floor->TransitionPoints.Add(TP); }
        Floor->MarkPackageDirty();
        return true;
    }

    // InteriorSet
    if (UInteriorSetAsset* Interior = Cast<UInteriorSetAsset>(RegAsset))
    {
        Interior->Modify();
        FLocationTransitionPoint* Existing = Interior->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing) { *Existing = TP; } else { Interior->TransitionPoints.Add(TP); }
        Interior->MarkPackageDirty();
        return true;
    }

    // Street
    if (UStreetAsset* Street = Cast<UStreetAsset>(RegAsset))
    {
        Street->Modify();
        FLocationTransitionPoint* Existing = Street->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing) { *Existing = TP; } else { Street->TransitionPoints.Add(TP); }
        Street->MarkPackageDirty();
        return true;
    }

    // WorldRegion — актор без улицы и дома
    if (UWorldRegionAsset* Region = Cast<UWorldRegionAsset>(RegAsset))
    {
        // Валидация массива перед добавлением: чистим висячие ссылки
        ValidateAndCleanTransitionPoints(Region);

        Region->Modify();
        FLocationTransitionPoint* Existing = Region->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing) { *Existing = TP; } else { Region->TransitionPoints.Add(TP); }
        Region->MarkPackageDirty();
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("RegisterTransitionPoint: unknown registration asset type '%s'"), *RegAsset->GetClass()->GetName());
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// RemoveTransitionPointFromAllAssets
// Вариант А: сканирует ВСЕ ассеты через AssetRegistry и удаляет запись по AnchorID.
// ─────────────────────────────────────────────────────────────────────────────
int32 ULocationEditorUtils::RemoveTransitionPointFromAllAssets(const FGuid& AnchorID)
{
    if (!AnchorID.IsValid()) return 0;

    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AR = ARM.Get();

    int32 RemovedTotal = 0;

    auto RemoveFromArray = [&](TArray<FLocationTransitionPoint>& Points, UObject* Owner) -> int32
    {
        const int32 Before = Points.Num();
        Points.RemoveAll([&](const FLocationTransitionPoint& P)
        {
            return P.TransitionPointID == AnchorID;
        });
        const int32 After = Points.Num();
        const int32 Removed = Before - After;
        if (Removed > 0)
        {
            Owner->Modify();
            Owner->MarkPackageDirty();
            UE_LOG(LogTemp, Log, TEXT("RemoveTransitionPointFromAllAssets: deleted %d records from '%s' (before=%d, after=%d)"),
                Removed, *Owner->GetName(), Before, After);
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("RemoveTransitionPointFromAllAssets: no records found in '%s' (checked %d)"),
                *Owner->GetName(), Before);
        }
        return Removed;
    };

    auto LoadAssetObject = [&](const FAssetData& AD) -> UObject*
    {
        UObject* Obj = AD.GetAsset();
        if (Obj) return Obj;

        // Попытка загрузить через SoftPath
        FSoftObjectPath SOP = AD.ToSoftObjectPath();
        if (SOP.IsValid())
        {
            Obj = SOP.TryLoad();
            if (Obj) return Obj;
        }

        // Как запасной вариант, попытаться LoadObject по ObjectPath
        const FString ObjPath = AD.ObjectPath.ToString();
        if (!ObjPath.IsEmpty())
        {
            Obj = LoadObject<UObject>(nullptr, *ObjPath);
            if (Obj) return Obj;
        }

        return nullptr;
    };

    // ── WorldRegionAsset ──────────────────────────────────────────────────
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UWorldRegionAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AR.GetAssets(Filter, Assets);

        for (const FAssetData& AD : Assets)
        {
            UObject* Loaded = LoadAssetObject(AD);
            if (!Loaded) continue;
            UWorldRegionAsset* Asset = Cast<UWorldRegionAsset>(Loaded);
            if (!Asset) continue;
            RemovedTotal += RemoveFromArray(Asset->TransitionPoints, Asset);
        }
    }

    // ── StreetAsset ───────────────────────────────────────────────────────
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UStreetAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AR.GetAssets(Filter, Assets);

        for (const FAssetData& AD : Assets)
        {
            UObject* Loaded = LoadAssetObject(AD);
            if (!Loaded) continue;
            UStreetAsset* Asset = Cast<UStreetAsset>(Loaded);
            if (!Asset) continue;
            RemovedTotal += RemoveFromArray(Asset->TransitionPoints, Asset);
        }
    }

    // ── InteriorSetAsset ──────────────────────────────────────────────────
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UInteriorSetAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AR.GetAssets(Filter, Assets);

        for (const FAssetData& AD : Assets)
        {
            UObject* Loaded = LoadAssetObject(AD);
            if (!Loaded) continue;
            UInteriorSetAsset* Asset = Cast<UInteriorSetAsset>(Loaded);
            if (!Asset) continue;
            RemovedTotal += RemoveFromArray(Asset->TransitionPoints, Asset);
        }
    }

    // ── FloorAsset ────────────────────────────────────────────────────────
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UFloorAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AR.GetAssets(Filter, Assets);

        for (const FAssetData& AD : Assets)
        {
            UObject* Loaded = LoadAssetObject(AD);
            if (!Loaded) continue;
            UFloorAsset* Asset = Cast<UFloorAsset>(Loaded);
            if (!Asset) continue;
            RemovedTotal += RemoveFromArray(Asset->TransitionPoints, Asset);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("RemoveTransitionPointFromAllAssets: a total of %d records deleted for AnchorID %s"), RemovedTotal, *AnchorID.ToString());
    return RemovedTotal;
}

// ─────────────────────────────────────────────────────────────────────────────

int32 ULocationEditorUtils::ValidateAndCleanTransitionPoints(UObject* Asset)
{
    if (!Asset) return 0;
    int32 RemovedTotal = 0;

    auto CheckAndRemove = [&](TArray<FLocationTransitionPoint>& Points, const TArray<FSoftObjectPath>& LevelPaths, UObject* OwnerAsset)
    {
        int32 Removed = 0;
        for (int32 i = Points.Num() - 1; i >= 0; --i)
        {
            const FGuid ID = Points[i].TransitionPointID;
            bool bFound = false;

            if (!Points[i].SourceAnchor.IsNull())
            {
                UObject* Obj = Points[i].SourceAnchor.Get();
                if (Obj)
                {
                    if (ALocationAnchorActor* A = Cast<ALocationAnchorActor>(Obj))
                    {
                        if (IsValid(A) && A->AnchorID == ID) bFound = true;
                    }
                }
            }

            if (!bFound)
            {
                for (const FSoftObjectPath& LP : LevelPaths)
                {
                    if (IsAnchorPresentOnLevel(ID, LP)) { bFound = true; break; }
                }
            }

            if (!bFound)
            {
                Points.RemoveAt(i);
                ++Removed;
            }
        }

        if (Removed > 0 && OwnerAsset) OwnerAsset->MarkPackageDirty();
        RemovedTotal += Removed;
    };

    if (UFloorAsset* Floor = Cast<UFloorAsset>(Asset))
    {
        if (!Floor->FloorLevel.IsNull())
        {
            TArray<FSoftObjectPath> Paths = { Floor->FloorLevel.ToSoftObjectPath() };
            CheckAndRemove(Floor->TransitionPoints, Paths, Floor);
        }
    }
    else if (UInteriorSetAsset* Interior = Cast<UInteriorSetAsset>(Asset))
    {
        TArray<FSoftObjectPath> Paths;
        if (!Interior->ParentStreet.IsNull())
        {
            if (UStreetAsset* S = Interior->ParentStreet.LoadSynchronous())
                if (!S->ParentWorldRegion.IsNull())
                    if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous())
                        if (!R->RegionLevel.IsNull())
                            Paths.Add(R->RegionLevel.ToSoftObjectPath());
        }
        for (const TSoftObjectPtr<UFloorAsset>& FR : Interior->Floors)
            if (FR.IsValid())
                Paths.Add(FR.ToSoftObjectPath());

        CheckAndRemove(Interior->TransitionPoints, Paths, Interior);
    }
    else if (UStreetAsset* Street = Cast<UStreetAsset>(Asset))
    {
        TArray<FSoftObjectPath> Paths;
        if (!Street->ParentWorldRegion.IsNull())
            if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous())
                if (!R->RegionLevel.IsNull())
                    Paths.Add(R->RegionLevel.ToSoftObjectPath());

        for (const TSoftObjectPtr<UInteriorSetAsset>& IR : Street->InteriorSets)
            if (IR.IsValid())
                if (UInteriorSetAsset* IS = IR.LoadSynchronous())
                    for (const TSoftObjectPtr<UFloorAsset>& FR : IS->Floors)
                        if (FR.IsValid())
                            Paths.Add(FR.ToSoftObjectPath());

        CheckAndRemove(Street->TransitionPoints, Paths, Street);
    }
    // WorldRegion — точки перехода без улицы и дома
    else if (UWorldRegionAsset* Region = Cast<UWorldRegionAsset>(Asset))
    {
        TArray<FSoftObjectPath> Paths;
        if (!Region->RegionLevel.IsNull())
            Paths.Add(Region->RegionLevel.ToSoftObjectPath());

        CheckAndRemove(Region->TransitionPoints, Paths, Region);
    }

    return RemovedTotal;
}

#endif // WITH_EDITOR