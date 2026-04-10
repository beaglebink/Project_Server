#include "LocationSystemUtils.h"
#include "WorldMapAsset.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"

#if WITH_EDITOR
#include "LocationEditorUtils.h"
#include "Misc/MessageDialog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SoftObjectPath.h"
#endif

// ──────────────────────────────────────────────────────────────────────────────
// FLocationAddress::ToDisplayString
// ──────────────────────────────────────────────────────────────────────────────

FString FLocationAddress::ToDisplayString() const
{
    TArray<FString> Parts;
    if (!RegionName.IsEmpty())   Parts.Add(RegionName.ToString());
    if (!StreetName.IsEmpty())   Parts.Add(StreetName.ToString());
    if (!BuildingName.IsEmpty()) Parts.Add(BuildingName.ToString());
    if (!FloorName.IsEmpty())    Parts.Add(FloorName.ToString());
    if (!ZoneName.IsEmpty())     Parts.Add(ZoneName.ToString());
    return FString::Join(Parts, TEXT(", "));
}

// ──────────────────────────────────────────────────────────────────────────────
// Адресация
// ──────────────────────────────────────────────────────────────────────────────

FLocationAddress ULocationSystemUtils::BuildAddressForInteriorSet(UInteriorSetAsset* InteriorSet)
{
    FLocationAddress Result;
    if (!IsValid(InteriorSet)) return Result;

    Result.BuildingName = InteriorSet->DisplayName.IsEmpty()
        ? FText::FromString(InteriorSet->AddressLine.ToString())
        : InteriorSet->DisplayName;

    if (UStreetAsset* Street = InteriorSet->ParentStreet.LoadSynchronous())
    {
        Result.StreetName = Street->DisplayName;

        if (UWorldRegionAsset* Region = Street->ParentWorldRegion.LoadSynchronous())
        {
            Result.RegionName = Region->DisplayName;

            if (UWorldMapAsset* Map = Region->ParentWorldMap.LoadSynchronous())
            {
                Result.WorldMapName = Map->DisplayName;
            }
        }
    }

    return Result;
}

FLocationAddress ULocationSystemUtils::BuildAddressForFloor(UFloorAsset* Floor)
{
    FLocationAddress Result;
    if (!IsValid(Floor)) return Result;

    Result.FloorName = Floor->DisplayName;

    if (UInteriorSetAsset* InteriorSet = Floor->ParentInteriorSet.LoadSynchronous())
    {
        Result = BuildAddressForInteriorSet(InteriorSet);
        Result.FloorName = Floor->DisplayName;
    }

    return Result;
}

FLocationAddress ULocationSystemUtils::BuildAddressForZone(
    const FLocationZone& Zone,
    UStreetAsset* OwnerStreet,
    UFloorAsset* OwnerFloor)
{
    FLocationAddress Result;
    Result.ZoneName = Zone.DisplayName;

    if (IsValid(OwnerFloor))
    {
        Result = BuildAddressForFloor(OwnerFloor);
        Result.ZoneName = Zone.DisplayName;
    }
    else if (IsValid(OwnerStreet))
    {
        Result.StreetName = OwnerStreet->DisplayName;
        if (UWorldRegionAsset* Region = OwnerStreet->ParentWorldRegion.LoadSynchronous())
        {
            Result.RegionName = Region->DisplayName;
            if (UWorldMapAsset* Map = Region->ParentWorldMap.LoadSynchronous())
            {
                Result.WorldMapName = Map->DisplayName;
            }
        }
    }

    return Result;
}

// ──────────────────────────────────────────────────────────────────────────────
// Поиск по ID
// ──────────────────────────────────────────────────────────────────────────────

UStreetAsset* ULocationSystemUtils::FindStreetByID(UWorldRegionAsset* Region, const FGuid& StreetID)
{
    if (!IsValid(Region)) return nullptr;

    for (const TSoftObjectPtr<UStreetAsset>& Ref : Region->Streets)
    {
        if (UStreetAsset* Street = Ref.LoadSynchronous())
        {
            if (Street->StreetID == StreetID)
            {
                return Street;
            }
        }
    }
    return nullptr;
}

UInteriorSetAsset* ULocationSystemUtils::FindInteriorSetByID(UStreetAsset* Street, const FGuid& InteriorSetID)
{
    if (!IsValid(Street)) return nullptr;

    for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Street->InteriorSets)
    {
        if (UInteriorSetAsset* Interior = Ref.LoadSynchronous())
        {
            if (Interior->InteriorSetID == InteriorSetID)
            {
                return Interior;
            }
        }
    }
    return nullptr;
}

UFloorAsset* ULocationSystemUtils::FindFloorByID(UInteriorSetAsset* InteriorSet, const FGuid& FloorID)
{
    if (!IsValid(InteriorSet)) return nullptr;

    for (const TSoftObjectPtr<UFloorAsset>& Ref : InteriorSet->Floors)
    {
        if (UFloorAsset* Floor = Ref.LoadSynchronous())
        {
            if (Floor->FloorID == FloorID)
            {
                return Floor;
            }
        }
    }
    return nullptr;
}

// ──────────────────────────────────────────────────────────────────────────────
// Валидация
// ──────────────────────────────────────────────────────────────────────────────

void ULocationSystemUtils::ValidateTransitionPoints(
    const TArray<FLocationTransitionPoint>& Points,
    const FString& OwnerContext,
    FLocationValidationResult& OutResult)
{
    for (const FLocationTransitionPoint& TP : Points)
    {
        if (!TP.TransitionPointID.IsValid())
        {
            OutResult.AddIssue(FString::Printf(
                TEXT("[%s] TransitionPoint имеет невалидный TransitionPointID"), *OwnerContext));
        }
        if (!TP.SourceLocationID.IsValid())
        {
            OutResult.AddIssue(FString::Printf(
                TEXT("[%s] TransitionPoint '%s': SourceLocationID не задан"),
                *OwnerContext, *TP.DisplayName.ToString()));
        }
        if (!TP.DestinationLocationID.IsValid())
        {
            OutResult.AddIssue(FString::Printf(
                TEXT("[%s] TransitionPoint '%s': DestinationLocationID не задан"),
                *OwnerContext, *TP.DisplayName.ToString()));
        }
        if (TP.SourceLocationID == TP.DestinationLocationID)
        {
            OutResult.AddIssue(FString::Printf(
                TEXT("[%s] TransitionPoint '%s': Source и Destination указывают на одну локацию"),
                *OwnerContext, *TP.DisplayName.ToString()));
        }
    }
}

FLocationValidationResult ULocationSystemUtils::ValidateFloor(UFloorAsset* Floor)
{
    FLocationValidationResult Result;
    if (!IsValid(Floor))
    {
        Result.AddIssue(TEXT("Floor: ассет равен nullptr"));
        return Result;
    }

    if (!Floor->FloorID.IsValid())
        Result.AddIssue(FString::Printf(TEXT("Floor '%s': FloorID не задан"), *Floor->GetName()));

    if (Floor->ParentInteriorSet.IsNull())
        Result.AddIssue(FString::Printf(TEXT("Floor '%s': ParentInteriorSet не задан"), *Floor->GetName()));

    const FString Ctx = Floor->GetName();
    ValidateTransitionPoints(Floor->TransitionPoints, Ctx, Result);

    return Result;
}

FLocationValidationResult ULocationSystemUtils::ValidateInteriorSet(UInteriorSetAsset* InteriorSet)
{
    FLocationValidationResult Result;
    if (!IsValid(InteriorSet))
    {
        Result.AddIssue(TEXT("InteriorSet: ассет равен nullptr"));
        return Result;
    }

    if (!InteriorSet->InteriorSetID.IsValid())
        Result.AddIssue(FString::Printf(TEXT("InteriorSet '%s': InteriorSetID не задан"), *InteriorSet->GetName()));

    if (InteriorSet->ParentStreet.IsNull())
        Result.AddIssue(FString::Printf(TEXT("InteriorSet '%s': ParentStreet не задан"), *InteriorSet->GetName()));

    if (InteriorSet->Floors.IsEmpty())
        Result.AddIssue(FString::Printf(TEXT("InteriorSet '%s': не содержит ни одного Floor"), *InteriorSet->GetName()));

    const FString Ctx = InteriorSet->GetName();
    ValidateTransitionPoints(InteriorSet->EntranceTransitionPoints, Ctx, Result);

    for (const TSoftObjectPtr<UFloorAsset>& FloorRef : InteriorSet->Floors)
    {
        if (FloorRef.IsNull())
        {
            Result.AddIssue(FString::Printf(
                TEXT("InteriorSet '%s': содержит null-ссылку на Floor"), *InteriorSet->GetName()));
            continue;
        }
        // Глубокая проверка этажей
        FLocationValidationResult FloorResult = ValidateFloor(FloorRef.LoadSynchronous());
        Result.Issues.Append(FloorResult.Issues);
        if (!FloorResult.bIsValid) Result.bIsValid = false;
    }

    return Result;
}

FLocationValidationResult ULocationSystemUtils::ValidateStreet(UStreetAsset* Street)
{
    FLocationValidationResult Result;
    if (!IsValid(Street))
    {
        Result.AddIssue(TEXT("Street: ассет равен nullptr"));
        return Result;
    }

    if (!Street->StreetID.IsValid())
        Result.AddIssue(FString::Printf(TEXT("Street '%s': StreetID не задан"), *Street->GetName()));

    if (Street->ParentWorldRegion.IsNull())
        Result.AddIssue(FString::Printf(TEXT("Street '%s': ParentWorldRegion не задан"), *Street->GetName()));

    const FString Ctx = Street->GetName();
    ValidateTransitionPoints(Street->TransitionPoints, Ctx, Result);

    for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Street->InteriorSets)
    {
        if (Ref.IsNull())
        {
            Result.AddIssue(FString::Printf(
                TEXT("Street '%s': содержит null-ссылку на InteriorSet"), *Street->GetName()));
            continue;
        }
        FLocationValidationResult IntResult = ValidateInteriorSet(Ref.LoadSynchronous());
        Result.Issues.Append(IntResult.Issues);
        if (!IntResult.bIsValid) Result.bIsValid = false;
    }

    return Result;
}

FLocationValidationResult ULocationSystemUtils::ValidateWorldMap(UWorldMapAsset* WorldMap)
{
    FLocationValidationResult Result;
    if (!IsValid(WorldMap))
    {
        Result.AddIssue(TEXT("WorldMap: ассет равен nullptr"));
        return Result;
    }

    if (!WorldMap->WorldMapID.IsValid())
        Result.AddIssue(FString::Printf(TEXT("WorldMap '%s': WorldMapID не задан"), *WorldMap->GetName()));

    for (const TSoftObjectPtr<UWorldRegionAsset>& RegionRef : WorldMap->Regions)
    {
        if (RegionRef.IsNull())
        {
            Result.AddIssue(FString::Printf(
                TEXT("WorldMap '%s': содержит null-ссылку на WorldRegion"), *WorldMap->GetName()));
            continue;
        }

        UWorldRegionAsset* Region = RegionRef.LoadSynchronous();
        if (!IsValid(Region)) continue;

        if (!Region->WorldRegionID.IsValid())
            Result.AddIssue(FString::Printf(TEXT("WorldRegion '%s': WorldRegionID не задан"), *Region->GetName()));

        if (Region->ParentWorldMap.IsNull())
            Result.AddIssue(FString::Printf(TEXT("WorldRegion '%s': ParentWorldMap не задан"), *Region->GetName()));

        for (const TSoftObjectPtr<UStreetAsset>& StreetRef : Region->Streets)
        {
            if (StreetRef.IsNull())
            {
                Result.AddIssue(FString::Printf(
                    TEXT("WorldRegion '%s': содержит null-ссылку на Street"), *Region->GetName()));
                continue;
            }
            FLocationValidationResult StreetResult = ValidateStreet(StreetRef.LoadSynchronous());
            Result.Issues.Append(StreetResult.Issues);
            if (!StreetResult.bIsValid) Result.bIsValid = false;
        }
    }

    return Result;
}

#if WITH_EDITOR
void ULocationSystemUtils::OpenFloorAssignerWindow()
{
	FText Msg = FText::FromString(
		TEXT("Floor Assigner helper:\n\n")
		TEXT("Open the Editor Utility Widget named 'FloorAssignerEditorWidget' via:\n")
		TEXT("  Window -> Developer Tools -> Editor Utility Widgets\n\n")
		TEXT("Create/open the widget instance and use provided Blueprint-callable functions:\n")
		TEXT("GetWorldMaps -> GetRegions -> GetStreets -> GetInteriorSets -> GetFloors\n")
		TEXT("Then call ApplyFloorToSelectedActors to write GUIDs to selected actors."));
	FMessageDialog::Open(EAppMsgType::Ok, Msg);
}

TArray<UWorldMapAsset*> ULocationEditorUtils::GetAllWorldMapAssets()
{
	TArray<UWorldMapAsset*> Result;

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;

	// Получаем все FAssetData класса WorldMapAsset
	ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("WorldMapAsset")), AssetDataList, true);

	for (const FAssetData& AD : AssetDataList)
	{
		// Попытка получить уже загруженный объект
		if (UObject* Obj = AD.GetAsset())
		{
			if (UWorldMapAsset* Map = Cast<UWorldMapAsset>(Obj))
			{
				Result.Add(Map);
				continue;
			}
		}

		// Если не загружен — загрузим через SoftObjectPath
		FSoftObjectPath SoftPath = AD.ToSoftObjectPath();
		if (UWorldMapAsset* Map = Cast<UWorldMapAsset>(SoftPath.TryLoad()))
		{
			Result.Add(Map);
		}
	}

	return Result;
}
#endif

