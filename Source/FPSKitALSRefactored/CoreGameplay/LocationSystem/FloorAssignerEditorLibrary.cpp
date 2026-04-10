#include "FloorAssignerEditorLibrary.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "WorldMapAsset.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "GameFramework/Actor.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "EngineUtils.h"
#include "Editor/EditorEngine.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteriorInstanceSystem/FloorPlacementPayload.h"

static TArray<FAssetData> GetAssetDataByClassName(const FString& ClassName)
{
	TArray<FAssetData> Result;
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), *ClassName), Result, true);
	return Result;
}

// Helper: resolve floor display name (use Floor->DisplayName if set, otherwise asset name)
// If not found, fall back to GUID string.
static FText ResolveFloorDisplayName(const FGuid& FloorGuid)
{
	if (!FloorGuid.IsValid())
	{
		return FText::FromString(TEXT("Invalid GUID"));
	}

	for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
	{
		FSoftObjectPath Path = AD.ToSoftObjectPath();
		UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
		if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
		if (!IS) continue;
		for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
		{
			UFloorAsset* Floor = Ref.LoadSynchronous();
			if (!Floor) continue;
			if (Floor->FloorID != FloorGuid) continue;

			if (!Floor->DisplayName.IsEmpty())
			{
				return Floor->DisplayName;
			}
			// fallback to asset name
			return FText::FromName(Floor->GetFName());
		}
	}
	// Not found: fallback to GUID string
	return FText::FromString(FloorGuid.ToString());
}

TMap<FGuid, FText> UFloorAssignerEditorLibrary::GetWorldMaps()
{
	TMap<FGuid, FText> Out;
	for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldMapAsset")))
	{
		FSoftObjectPath Path = AD.ToSoftObjectPath();

		UWorldMapAsset* Map = Cast<UWorldMapAsset>(AD.GetAsset());
		if (!Map)
		{
			Map = Cast<UWorldMapAsset>(Path.TryLoad());
		}

		if (Map && Map->WorldMapID.IsValid())
		{
			Out.Add(Map->WorldMapID, Map->DisplayName.IsEmpty() ? FText::FromName(Map->GetFName()) : Map->DisplayName);
		}
	}
	return Out;
}

TMap<FGuid, FText> UFloorAssignerEditorLibrary::GetRegions(const FGuid& WorldMapGuid)
{
	TMap<FGuid, FText> Out;
	// find the world map
	for (UWorldMapAsset* Map : TArray<UWorldMapAsset*>([&]()->TArray<UWorldMapAsset*>
	{
		TArray<UWorldMapAsset*> R;
		for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldMapAsset")))
		{
			FSoftObjectPath Path = AD.ToSoftObjectPath();
			if (UWorldMapAsset* A = Cast<UWorldMapAsset>(AD.GetAsset()))
			{
				R.Add(A);
			}
			else if (UWorldMapAsset* A2 = Cast<UWorldMapAsset>(Path.TryLoad()))
			{
				R.Add(A2);
			}
		}
		return R;
	}()))
	{
		if (!Map) continue;
		if (Map->WorldMapID != WorldMapGuid) continue;

		for (const TSoftObjectPtr<UWorldRegionAsset>& Ref : Map->Regions)
		{
			UWorldRegionAsset* Region = Ref.LoadSynchronous();
			if (!Region) continue;
			if (!Region->WorldRegionID.IsValid()) continue;
			Out.Add(Region->WorldRegionID, Region->DisplayName.IsEmpty() ? FText::FromName(Region->GetFName()) : Region->DisplayName);
		}
		break;
	}
	return Out;
}

TMap<FGuid, FText> UFloorAssignerEditorLibrary::GetStreets(const FGuid& WorldRegionGuid)
{
	TMap<FGuid, FText> Out;
	// find region
	for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldRegionAsset")))
	{
		FSoftObjectPath Path = AD.ToSoftObjectPath();
		UWorldRegionAsset* Region = Cast<UWorldRegionAsset>(AD.GetAsset());
		if (!Region) Region = Cast<UWorldRegionAsset>(Path.TryLoad());
		if (!Region) continue;
		if (Region->WorldRegionID != WorldRegionGuid) continue;

		for (const TSoftObjectPtr<UStreetAsset>& Ref : Region->Streets)
		{
			UStreetAsset* Street = Ref.LoadSynchronous();
			if (!Street) continue;
			if (!Street->StreetID.IsValid()) continue;
			Out.Add(Street->StreetID, Street->DisplayName.IsEmpty() ? FText::FromName(Street->GetFName()) : Street->DisplayName);
		}
		break;
	}
	return Out;
}

TMap<FGuid, FText> UFloorAssignerEditorLibrary::GetInteriorSets(const FGuid& StreetGuid)
{
	TMap<FGuid, FText> Out;
	for (const FAssetData& AD : GetAssetDataByClassName(TEXT("StreetAsset")))
	{
		FSoftObjectPath Path = AD.ToSoftObjectPath();
		UStreetAsset* Street = Cast<UStreetAsset>(AD.GetAsset());
		if (!Street) Street = Cast<UStreetAsset>(Path.TryLoad());
		if (!Street) continue;
		if (Street->StreetID != StreetGuid) continue;

		for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Street->InteriorSets)
		{
			UInteriorSetAsset* IS = Ref.LoadSynchronous();
			if (!IS) continue;
			if (!IS->InteriorSetID.IsValid()) continue;
			Out.Add(IS->InteriorSetID, IS->DisplayName.IsEmpty() ? FText::FromName(IS->GetFName()) : IS->DisplayName);
		}
		break;
	}
	return Out;
}

TMap<FGuid, FText> UFloorAssignerEditorLibrary::GetFloors(const FGuid& InteriorSetGuid)
{
	TMap<FGuid, FText> Out;
	for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
	{
		FSoftObjectPath Path = AD.ToSoftObjectPath();
		UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
		if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
		if (!IS) continue;
		if (IS->InteriorSetID != InteriorSetGuid) continue;

		for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
		{
			UFloorAsset* Floor = Ref.LoadSynchronous();
			if (!Floor) continue;
			if (!Floor->FloorID.IsValid()) continue;
			Out.Add(Floor->FloorID, Floor->DisplayName.IsEmpty() ? FText::FromName(Floor->GetFName()) : Floor->DisplayName);
		}
		break;
	}
	return Out;
}

int32 UFloorAssignerEditorLibrary::ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid)
{
	int32 ModifiedCount = 0;
	if (!GEditor) return 0;
	USelection* Selected = GEditor->GetSelectedActors();
	if (!Selected) return 0;

	// Resolve display names for InteriorSet and Floor to write into components
	FText ResInteriorSetName = FText::GetEmpty();
	FText ResFloorName = FText::GetEmpty();

	for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
	{
		FSoftObjectPath Path = AD.ToSoftObjectPath();
		UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
		if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
		if (!IS) continue;
		if (IS->InteriorSetID != InteriorSetGuid) continue;

		ResInteriorSetName = IS->DisplayName.IsEmpty() ? FText::FromName(IS->GetFName()) : IS->DisplayName;

		// find floor inside this interior set
		for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
		{
			UFloorAsset* Floor = Ref.LoadSynchronous();
			if (!Floor) continue;
			if (Floor->FloorID != FloorGuid) continue;

			ResFloorName = Floor->DisplayName.IsEmpty()
				? FText::FromName(Floor->GetFName())
				: FText::Format(FText::FromString(TEXT("{0} — {1}")), FText::AsNumber(Floor->FloorIndex), Floor->DisplayName);
			break;
		}
		break;
	}

	TArray<AActor*> Actors;
	Selected->GetSelectedObjects<AActor>(Actors);

	for (AActor* A : Actors)
	{
		if (!IsValid(A)) continue;
		if (UFloorAssignmentComponent* Comp = A->FindComponentByClass<UFloorAssignmentComponent>())
		{
			A->Modify();
			Comp->Modify();
			Comp->FloorId = FloorGuid;
			Comp->InteriorSetId = InteriorSetGuid;
			// записываем имена (если найдены)
			Comp->InteriorSetName = ResInteriorSetName;
			Comp->FloorName = ResFloorName;

			if (UPackage* Pkg = A->GetOutermost())
			{
				Pkg->MarkPackageDirty();
			}
			++ModifiedCount;
		}
	}
	// Use floor display name for the message (fallback to GUID string inside ResolveFloorDisplayName)
	FText Display = ResolveFloorDisplayName(FloorGuid);
	FMessageDialog::Open(EAppMsgType::Ok, FText::Format(FText::FromString(TEXT("Assigned '{0}' to {1} actors.")), Display, FText::AsNumber(ModifiedCount)));
	return ModifiedCount;
}

#endif // WITH_EDITOR

// -----------------------------------------------------------------------------
// Select actors by Floor GUID (editor-only)
// -----------------------------------------------------------------------------
#if WITH_EDITOR
int32 UFloorAssignerEditorLibrary::SelectActorsByFloor(const FGuid& FloorGuid)
{
	int32 Count = 0;
	if (!GEditor || !GEngine) return 0;

	// Deselect everything first
	GEditor->SelectNone(false, true, false);

	// Iterate all engine world contexts (editor + PIE + editor preview)
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& WC : WorldContexts)
	{
		UWorld* World = WC.World();
		if (!World) continue;

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!IsValid(Actor)) continue;
			UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
			if (!Comp) continue;
			if (Comp->GetFloorId() == FloorGuid)
			{
				GEditor->SelectActor(Actor, true, false, true);
				++Count;
			}
		}
	}

	// Notify editor about selection change
	GEditor->NoteSelectionChange();

	FText Display = ResolveFloorDisplayName(FloorGuid);
	FMessageDialog::Open(EAppMsgType::Ok, FText::Format(FText::FromString(TEXT("Selected {0} actors for floor '{1}'")), FText::AsNumber(Count), Display));

	return Count;
}
#endif // WITH_EDITOR

// -----------------------------------------------------------------------------
// Unregister selected actors (editor-only)
// -----------------------------------------------------------------------------
#if WITH_EDITOR
int32 UFloorAssignerEditorLibrary::UnregisterSelectedActors()
{
	int32 Count = 0;
	if (!GEditor) return 0;

	USelection* Selected = GEditor->GetSelectedActors();
	if (!Selected) return 0;

	TArray<AActor*> Actors;
	Selected->GetSelectedObjects<AActor>(Actors);

	for (AActor* A : Actors)
	{
		if (!IsValid(A)) continue;

		UFloorAssignmentComponent* Comp = A->FindComponentByClass<UFloorAssignmentComponent>();
		if (!Comp) continue;

		// Теперь снимаем регистрацию — инвалидируем GUID этажа и очищаем имя этажа
		// Модифицируем компонент и актора, помечаем пакет dirty
		Comp->Modify();
		A->Modify();

		Comp->FloorId.Invalidate();                 // обнуляем GUID этажа
		Comp->FloorName = FText::GetEmpty();        // очищаем отображаемое имя

		if (UPackage* Pkg = A->GetOutermost())
		{
			Pkg->MarkPackageDirty();
		}

		++Count;
	}

	FText Msg = FText::Format(FText::FromString(TEXT("Unregistered and cleared floor for {0} selected actors.")), FText::AsNumber(Count));
	FMessageDialog::Open(EAppMsgType::Ok, Msg);

	return Count;
}
#endif // WITH_EDITOR