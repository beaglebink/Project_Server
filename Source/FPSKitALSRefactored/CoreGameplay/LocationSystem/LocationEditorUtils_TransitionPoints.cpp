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

/** Находит актор-янкор по AnchorID на уровне LevelSoftPath (если загружен пакет) */
static ALocationAnchorActor* FindAnchorOnLevel(const FGuid& AnchorID, const FSoftObjectPath& LevelSoftPath)
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

    UE_LOG(LogTemp, Log, TEXT("RegisterTransitionPoint: actor='%s' AnchorID=%s OwnerContext=%d"),
        *SourceAnchor->GetName(), *SourceAnchor->AnchorID.ToString(), (int)SourceAnchor->OwnerContextType);

    // Log owner fields (paths)
    const FString OwnerRegionPath = SourceAnchor->OwnerRegion.IsNull() ? TEXT("null") : SourceAnchor->OwnerRegion.ToSoftObjectPath().GetAssetPathString();
    const FString OwnerStreetPath = SourceAnchor->OwnerStreet.IsNull() ? TEXT("null") : SourceAnchor->OwnerStreet.ToSoftObjectPath().GetAssetPathString();
    const FString OwnerInteriorPath = SourceAnchor->OwnerInteriorSet.IsNull() ? TEXT("null") : SourceAnchor->OwnerInteriorSet.ToSoftObjectPath().GetAssetPathString();
    const FString OwnerFloorPath = SourceAnchor->OwnerFloor.IsNull() ? TEXT("null") : SourceAnchor->OwnerFloor.ToSoftObjectPath().GetAssetPathString();

    UE_LOG(LogTemp, Log, TEXT("  OwnerRegion=%s OwnerStreet=%s OwnerInteriorSet=%s OwnerFloor=%s"),
        *OwnerRegionPath, *OwnerStreetPath, *OwnerInteriorPath, *OwnerFloorPath);

    // Построим запись TP из актора
    FLocationTransitionPoint TP;
    TP.TransitionPointID       = SourceAnchor->AnchorID;
    TP.DisplayName             = SourceAnchor->DisplayName.IsEmpty() ? FText::FromString(SourceAnchor->GetName()) : SourceAnchor->DisplayName;
    TP.TransitionType          = SourceAnchor->TransitionType;
    TP.SourceAnchor            = TSoftObjectPtr<UObject>(SourceAnchor);
    TP.SourceWorldPosition     = SourceAnchor->GetActorLocation();
    TP.SourceWorldOrientation  = SourceAnchor->GetActorRotation();
    TP.DestinationLink         = SourceAnchor->DestinationLink;

    // Попробуем найти актор назначения в Level (и заполнить DestinationAnchor / TargetAnchorActor)
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
            if (ALocationAnchorActor* DestA = FindAnchorOnLevel(TP.DestinationLink.TargetAnchorID, DestLevelPath))
            {
                TP.DestinationAnchor = TSoftObjectPtr<UObject>(DestA);
                TP.DestinationLink.TargetAnchorActor = TSoftObjectPtr<UObject>(DestA);
                UE_LOG(LogTemp, Log, TEXT("  Found destination actor '%s' for TargetAnchorID=%s"), *DestA->GetName(), *TP.DestinationLink.TargetAnchorID.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Verbose, TEXT("  Destination actor not found on level for TargetAnchorID=%s"), *TP.DestinationLink.TargetAnchorID.ToString());
            }
        }
    }

    // Determine registration asset
    UObject* RegAsset = SourceAnchor->GetOwnerRegistrationAsset();
    if (!RegAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("RegisterTransitionPoint: GetOwnerRegistrationAsset returned nullptr for actor '%s'"), *SourceAnchor->GetName());
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RegisterTransitionPoint: registering into asset '%s' (%s)"),
        *RegAsset->GetName(), *RegAsset->GetClass()->GetName());

    // Floor
    if (UFloorAsset* Floor = Cast<UFloorAsset>(RegAsset))
    {
        Floor->Modify();
        FLocationTransitionPoint* Existing = Floor->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing)
        {
            *Existing = TP;
            UE_LOG(LogTemp, Log, TEXT("  Updated existing TransitionPoint in Floor '%s'"), *Floor->GetName());
        }
        else
        {
            Floor->TransitionPoints.Add(TP);
            UE_LOG(LogTemp, Log, TEXT("  Added TransitionPoint to Floor '%s'"), *Floor->GetName());
        }
        Floor->MarkPackageDirty();
        return true;
    }

    // InteriorSet
    if (UInteriorSetAsset* Interior = Cast<UInteriorSetAsset>(RegAsset))
    {
        Interior->Modify();
        FLocationTransitionPoint* Existing = Interior->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing)
        {
            *Existing = TP;
            UE_LOG(LogTemp, Log, TEXT("  Updated existing TransitionPoint in InteriorSet '%s'"), *Interior->GetName());
        }
        else
        {
            Interior->TransitionPoints.Add(TP);
            UE_LOG(LogTemp, Log, TEXT("  Added TransitionPoint to InteriorSet '%s'"), *Interior->GetName());
        }
        Interior->MarkPackageDirty();
        return true;
    }

    // Street
    if (UStreetAsset* Street = Cast<UStreetAsset>(RegAsset))
    {
        Street->Modify();
        FLocationTransitionPoint* Existing = Street->TransitionPoints.FindByPredicate(
            [&](const FLocationTransitionPoint& P) { return P.TransitionPointID == TP.TransitionPointID; });
        if (Existing)
        {
            *Existing = TP;
            UE_LOG(LogTemp, Log, TEXT("  Updated existing TransitionPoint in Street '%s'"), *Street->GetName());
        }
        else
        {
            Street->TransitionPoints.Add(TP);
            UE_LOG(LogTemp, Log, TEXT("  Added TransitionPoint to Street '%s'"), *Street->GetName());
        }
        Street->MarkPackageDirty();
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT("RegisterTransitionPoint: unknown registration asset type '%s'"), *RegAsset->GetClass()->GetName());
    return false;
}

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

            // Если SourceAnchor soft ptr валиден — быстрая проверка
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

            // Иначе сканируем связные уровни
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
            {
                if (!S->ParentWorldRegion.IsNull())
                {
                    if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous())
                    {
                        if (!R->RegionLevel.IsNull())
                            Paths.Add(R->RegionLevel.ToSoftObjectPath());
                    }
                }
            }
        }

        for (const TSoftObjectPtr<UFloorAsset>& FR : Interior->Floors)
        {
            if (FR.IsValid())
            {
                Paths.Add(FR.ToSoftObjectPath());
            }
        }

        CheckAndRemove(Interior->TransitionPoints, Paths, Interior);
    }
    else if (UStreetAsset* Street = Cast<UStreetAsset>(Asset))
    {
        TArray<FSoftObjectPath> Paths;
        if (!Street->ParentWorldRegion.IsNull())
        {
            if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous())
            {
                if (!R->RegionLevel.IsNull())
                    Paths.Add(R->RegionLevel.ToSoftObjectPath());
            }
        }

        for (const TSoftObjectPtr<UInteriorSetAsset>& IR : Street->InteriorSets)
        {
            if (IR.IsValid())
            {
                if (UInteriorSetAsset* IS = IR.LoadSynchronous())
                {
                    for (const TSoftObjectPtr<UFloorAsset>& FR : IS->Floors)
                    {
                        if (FR.IsValid())
                            Paths.Add(FR.ToSoftObjectPath());
                    }
                }
            }
        }

        CheckAndRemove(Street->TransitionPoints, Paths, Street);
    }

    return RemovedTotal;
}

int32 ULocationEditorUtils::ValidateAllTransitionPoints()
{
    int32 TotalRemoved = 0;
    FAssetRegistryModule& ARModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AR = ARModule.Get();

    auto ScanClass = [&](const FTopLevelAssetPath& ClassPath)
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(ClassPath);
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AR.GetAssets(Filter, Assets);
        for (const FAssetData& AD : Assets)
        {
            if (UObject* Obj = AD.GetAsset())
                TotalRemoved += ValidateAndCleanTransitionPoints(Obj);
        }
    };

    ScanClass(UFloorAsset::StaticClass()->GetClassPathName());
    ScanClass(UInteriorSetAsset::StaticClass()->GetClassPathName());
    ScanClass(UStreetAsset::StaticClass()->GetClassPathName());

    UE_LOG(LogTemp, Log, TEXT("ValidateAllTransitionPoints: total removed %d"), TotalRemoved);
    return TotalRemoved;
}

#endif // WITH_EDITOR