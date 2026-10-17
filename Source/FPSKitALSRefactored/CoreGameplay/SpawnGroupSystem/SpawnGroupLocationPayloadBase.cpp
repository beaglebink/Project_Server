// SpawnGroupLocationPayloadBase.cpp
#include "SpawnGroupLocationPayloadBase.h"

#if WITH_EDITOR

#include "WorldMapAsset.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"
#include "SpawnGroupSpawner.h"
#include "FloorAssignmentComponent.h"
#include "UObject/Package.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"

static UWorld* LoadLevelWorld(const FString& PackageName)
{
    if (PackageName.IsEmpty()) return nullptr;
    UPackage* Pkg = FindPackage(nullptr, *PackageName);
    if (!Pkg) Pkg = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Pkg) return nullptr;
    return UWorld::FindWorldInPackage(Pkg);
}

// ─────────────────────────────────────────────────────────────────────────────
// 1. Заполнение выпадающих списков (строго по иерархии)
// ─────────────────────────────────────────────────────────────────────────────

TArray<FString> USpawnGroupLocationPayloadBase::GetRegionOptions() const
{
    TArray<FString> Result;
    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    ARM.Get().GetAssetsByClass(UWorldMapAsset::StaticClass()->GetClassPathName(), Assets, true);

    for (const FAssetData& AD : Assets)
    {
        UWorldMapAsset* Region = Cast<UWorldMapAsset>(AD.GetAsset());
        if (!Region) Region = Cast<UWorldMapAsset>(AD.ToSoftObjectPath().TryLoad());
        if (!Region) continue;

        Result.Add(Region->DisplayName.IsEmpty() ? Region->GetName() : Region->DisplayName.ToString());
    }
    return Result;
}

TArray<FString> USpawnGroupLocationPayloadBase::GetDistrictOptions() const
{
    TArray<FString> Result;
    UWorldMapAsset* Region = FindRegion();
    if (!Region) return Result;

    for (const TSoftObjectPtr<UWorldRegionAsset>& Ref : Region->Regions)
    {
        UWorldRegionAsset* District = Ref.LoadSynchronous();
        if (!District) continue;
        Result.Add(District->DisplayName.IsEmpty() ? District->GetName() : District->DisplayName.ToString());
    }
    return Result;
}

TArray<FString> USpawnGroupLocationPayloadBase::GetStreetOptions() const
{
    TArray<FString> Result;
    UWorldMapAsset* Region = FindRegion();
    UWorldRegionAsset* District = Region ? FindDistrict(Region) : nullptr;
    if (!District) return Result;

    for (const TSoftObjectPtr<UStreetAsset>& Ref : District->Streets)
    {
        UStreetAsset* Street = Ref.LoadSynchronous();
        if (!Street) continue;
        Result.Add(Street->DisplayName.IsEmpty() ? Street->GetName() : Street->DisplayName.ToString());
    }
    return Result;
}

TArray<FString> USpawnGroupLocationPayloadBase::GetBuildingOptions() const
{
    TArray<FString> Result;
    UWorldMapAsset* Region = FindRegion();
    UWorldRegionAsset* District = Region ? FindDistrict(Region) : nullptr;
    UStreetAsset* Street = District ? FindStreet(District) : nullptr;
    if (!Street) return Result;

    for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Street->InteriorSets)
    {
        UInteriorSetAsset* IS = Ref.LoadSynchronous();
        if (!IS) continue;
        Result.Add(IS->DisplayName.IsEmpty() ? IS->GetName() : IS->DisplayName.ToString());
    }
    return Result;
}

TArray<FString> USpawnGroupLocationPayloadBase::GetFloorOptions() const
{
    TArray<FString> Result;
    UWorldMapAsset* Region = FindRegion();
    UWorldRegionAsset* District = Region ? FindDistrict(Region) : nullptr;
    UStreetAsset* Street = District ? FindStreet(District) : nullptr;
    UInteriorSetAsset* IS = Street ? FindBuilding(Street) : nullptr;
    if (!IS) return Result;

    for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
    {
        UFloorAsset* Floor = Ref.LoadSynchronous();
        if (!Floor) continue;
        Result.Add(Floor->DisplayName.IsEmpty() ? Floor->GetName() : Floor->DisplayName.ToString());
    }
    return Result;
}

TArray<FString> USpawnGroupLocationPayloadBase::GetSpawnerOptions() const
{
    TArray<FString> Result;
    UWorldMapAsset* Region = FindRegion();
    UWorldRegionAsset* District = Region ? FindDistrict(Region) : nullptr;
    UStreetAsset* Street = District ? FindStreet(District) : nullptr;
    UInteriorSetAsset* IS = Street ? FindBuilding(Street) : nullptr;
    UFloorAsset* Floor = IS ? FindFloor(IS) : nullptr;
    if (!Floor || Floor->FloorLevel.IsNull()) return Result;

    const FString LevelPkg = Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName();
    UWorld* World = LoadLevelWorld(LevelPkg);
    if (!World || !World->PersistentLevel) return Result;

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        ASpawnGroupSpawner* Spawner = Cast<ASpawnGroupSpawner>(Actor);
        if (!Spawner) continue;

        UFloorAssignmentComponent* Comp = Spawner->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp || !Comp->ItemId.IsValid()) continue;

        // Для акторов уровня используем их уникальное имя
        Result.Add(Spawner->GetName());
    }
    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Поиск ассетов по выбранным строкам (сверху вниз)
// ─────────────────────────────────────────────────────────────────────────────

UWorldMapAsset* USpawnGroupLocationPayloadBase::FindRegion() const
{
    if (TargetRegion.IsEmpty()) return nullptr;
    FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    ARM.Get().GetAssetsByClass(UWorldMapAsset::StaticClass()->GetClassPathName(), Assets, true);

    for (const FAssetData& AD : Assets)
    {
        UWorldMapAsset* Region = Cast<UWorldMapAsset>(AD.GetAsset());
        if (!Region) Region = Cast<UWorldMapAsset>(AD.ToSoftObjectPath().TryLoad());
        if (!Region) continue;

        FString Name = Region->DisplayName.IsEmpty() ? Region->GetName() : Region->DisplayName.ToString();
        if (Name == TargetRegion) return Region;
    }
    return nullptr;
}

UWorldRegionAsset* USpawnGroupLocationPayloadBase::FindDistrict(UWorldMapAsset* Region) const
{
    if (!Region || TargetDistrict.IsEmpty()) return nullptr;
    for (const TSoftObjectPtr<UWorldRegionAsset>& Ref : Region->Regions)
    {
        UWorldRegionAsset* District = Ref.LoadSynchronous();
        if (!District) continue;
        FString Name = District->DisplayName.IsEmpty() ? District->GetName() : District->DisplayName.ToString();
        if (Name == TargetDistrict) return District;
    }
    return nullptr;
}

UStreetAsset* USpawnGroupLocationPayloadBase::FindStreet(UWorldRegionAsset* District) const
{
    if (!District || TargetStreet.IsEmpty()) return nullptr;
    for (const TSoftObjectPtr<UStreetAsset>& Ref : District->Streets)
    {
        UStreetAsset* Street = Ref.LoadSynchronous();
        if (!Street) continue;
        FString Name = Street->DisplayName.IsEmpty() ? Street->GetName() : Street->DisplayName.ToString();
        if (Name == TargetStreet) return Street;
    }
    return nullptr;
}

UInteriorSetAsset* USpawnGroupLocationPayloadBase::FindBuilding(UStreetAsset* Street) const
{
    if (!Street || TargetBuilding.IsEmpty()) return nullptr;
    for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Street->InteriorSets)
    {
        UInteriorSetAsset* IS = Ref.LoadSynchronous();
        if (!IS) continue;
        FString Name = IS->DisplayName.IsEmpty() ? IS->GetName() : IS->DisplayName.ToString();
        if (Name == TargetBuilding) return IS;
    }
    return nullptr;
}

UFloorAsset* USpawnGroupLocationPayloadBase::FindFloor(UInteriorSetAsset* IS) const
{
    if (!IS || TargetFloor.IsEmpty()) return nullptr;
    for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
    {
        UFloorAsset* Floor = Ref.LoadSynchronous();
        if (!Floor) continue;
        FString Name = Floor->DisplayName.IsEmpty() ? Floor->GetName() : Floor->DisplayName.ToString();
        if (Name == TargetFloor) return Floor;
    }
    return nullptr;
}

ASpawnGroupSpawner* USpawnGroupLocationPayloadBase::FindSpawner(UFloorAsset* Floor) const
{
    if (!Floor || TargetSpawner.IsEmpty() || Floor->FloorLevel.IsNull()) return nullptr;
    const FString LevelPkg = Floor->FloorLevel.ToSoftObjectPath().GetLongPackageName();
    UWorld* World = LoadLevelWorld(LevelPkg);
    if (!World || !World->PersistentLevel) return nullptr;

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (IsValid(Actor) && Actor->GetName() == TargetSpawner)
        {
            return Cast<ASpawnGroupSpawner>(Actor);
        }
    }
    return nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. Реакция на изменение (очистка нижних уровней + запись GroupId)
// ─────────────────────────────────────────────────────────────────────────────

void USpawnGroupLocationPayloadBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    const FName Prop = PropertyChangedEvent.GetPropertyName();

    // Если изменился любой уровень, кроме самого спавнера — сбрасываем всех потомков
    if (Prop == GET_MEMBER_NAME_CHECKED(USpawnGroupLocationPayloadBase, TargetRegion))
    {
        Modify(); TargetDistrict.Empty(); TargetStreet.Empty(); TargetBuilding.Empty(); TargetFloor.Empty(); TargetSpawner.Empty();
        GroupId.Invalidate(); MarkPackageDirty();
    }
    else if (Prop == GET_MEMBER_NAME_CHECKED(USpawnGroupLocationPayloadBase, TargetDistrict))
    {
        Modify(); TargetStreet.Empty(); TargetBuilding.Empty(); TargetFloor.Empty(); TargetSpawner.Empty();
        GroupId.Invalidate(); MarkPackageDirty();
    }
    else if (Prop == GET_MEMBER_NAME_CHECKED(USpawnGroupLocationPayloadBase, TargetStreet))
    {
        Modify(); TargetBuilding.Empty(); TargetFloor.Empty(); TargetSpawner.Empty();
        GroupId.Invalidate(); MarkPackageDirty();
    }
    else if (Prop == GET_MEMBER_NAME_CHECKED(USpawnGroupLocationPayloadBase, TargetBuilding))
    {
        Modify(); TargetFloor.Empty(); TargetSpawner.Empty();
        GroupId.Invalidate(); MarkPackageDirty();
    }
    else if (Prop == GET_MEMBER_NAME_CHECKED(USpawnGroupLocationPayloadBase, TargetFloor))
    {
        Modify(); TargetSpawner.Empty();
        GroupId.Invalidate(); MarkPackageDirty();
    }
    // Если выбран спавнер — достаём его ItemId
    else if (Prop == GET_MEMBER_NAME_CHECKED(USpawnGroupLocationPayloadBase, TargetSpawner))
    {
        Modify();
        if (TargetSpawner.IsEmpty())
        {
            GroupId.Invalidate();
        }
        else
        {
            UWorldMapAsset* Region = FindRegion();
            UWorldRegionAsset* District = Region ? FindDistrict(Region) : nullptr;
            UStreetAsset* Street = District ? FindStreet(District) : nullptr;
            UInteriorSetAsset* IS = Street ? FindBuilding(Street) : nullptr;
            UFloorAsset* Floor = IS ? FindFloor(IS) : nullptr;
            ASpawnGroupSpawner* Spawner = Floor ? FindSpawner(Floor) : nullptr;

            if (Spawner)
            {
                if (UFloorAssignmentComponent* Comp = Spawner->FindComponentByClass<UFloorAssignmentComponent>())
                {
                    GroupId = Comp->ItemId;
                }
            }
            else
            {
                GroupId.Invalidate();
            }
        }
        MarkPackageDirty();
    }
}

#endif // WITH_EDITOR