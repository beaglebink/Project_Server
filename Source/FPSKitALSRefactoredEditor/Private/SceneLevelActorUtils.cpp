#include "SceneLevelActorUtils.h"

#include "FloorAssignmentComponent.h"
#include "InteractiveItemComponent.h"
#include "FloorAsset.h"
#include "WorldRegionAsset.h"

#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

UWorld* FSceneLevelActorUtils::ResolveWorldFromLevelPath(const FSoftObjectPath& LevelPath)
{
    if (!LevelPath.IsValid())
        return nullptr;

    const FString PackageName = LevelPath.GetLongPackageName();
    if (PackageName.IsEmpty())
        return nullptr;

    UPackage* Package = FindPackage(nullptr, *PackageName);
    if (!Package)
        Package = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Package)
        return nullptr;

    return UWorld::FindWorldInPackage(Package);
}

UWorld* FSceneLevelActorUtils::ResolveWorldFromSceneRef(
    ESceneAssetKind Kind,
    const TSoftObjectPtr<UFloorAsset>& Floor,
    const TSoftObjectPtr<UWorldRegionAsset>& Region)
{
    if (Kind == ESceneAssetKind::Floor)
    {
        if (UFloorAsset* F = Floor.LoadSynchronous())
        {
            if (!F->FloorLevel.IsNull())
                return ResolveWorldFromLevelPath(F->FloorLevel.ToSoftObjectPath());
        }
    }
    else if (Kind == ESceneAssetKind::Region)
    {
        if (UWorldRegionAsset* R = Region.LoadSynchronous())
        {
            if (!R->RegionLevel.IsNull())
                return ResolveWorldFromLevelPath(R->RegionLevel.ToSoftObjectPath());
        }
    }
    return nullptr;
}

void FSceneLevelActorUtils::CollectActors(
    UWorld* World,
    TFunctionRef<bool(AActor*)> Predicate,
    TArray<AActor*>& OutActors)
{
    OutActors.Reset();
    if (!World || !World->PersistentLevel)
        return;

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor))
            continue;
        if (Predicate(Actor))
            OutActors.Add(Actor);
    }
}

bool FSceneLevelActorUtils::HasFloorAssignment(AActor* Actor)
{
    if (!IsValid(Actor)) return false;
    UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>();
    return FAC && FAC->ItemId.IsValid();
}

bool FSceneLevelActorUtils::HasInteractiveItem(AActor* Actor)
{
    if (!IsValid(Actor)) return false;
    return Actor->FindComponentByClass<UInteractiveItemComponent>() != nullptr;
}