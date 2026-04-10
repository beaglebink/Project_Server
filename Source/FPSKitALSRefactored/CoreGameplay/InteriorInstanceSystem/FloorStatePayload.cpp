#include "FloorStatePayload.h"

#if WITH_EDITOR || WITH_ENGINE || WITH_SERVER_CODE
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "../LocationSystem/FloorAsset.h"
#include "../LocationSystem/InteriorSetAsset.h"
#include "UObject/SoftObjectPath.h"
#endif

UFloorStatePayload* UFloorStatePayload::Setup(const FString& InInteriorSetPathOrName, int32 InFloorIndex)
{
	InteriorSetPath = InInteriorSetPathOrName;
	FloorIndex = InFloorIndex;

	InteriorSetId.Invalidate();
	FloorId.Invalidate();

	if (InInteriorSetPathOrName.IsEmpty() || InFloorIndex < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: empty path/name or invalid floor index"));
		return this;
	}

#if WITH_EDITOR || WITH_ENGINE || WITH_SERVER_CODE
	// Query AssetRegistry for all InteriorSet assets (cached by AssetRegistry module)
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetDataList;
	ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("InteriorSetAsset")), AssetDataList, true);

	const FAssetData* Chosen = nullptr;

	// 1) Try exact SoftObjectPath match
	for (const FAssetData& AD : AssetDataList)
	{
		const FString SoftPath = AD.ToSoftObjectPath().ToString();
		if (SoftPath.Equals(InInteriorSetPathOrName, ESearchCase::IgnoreCase))
		{
			Chosen = &AD;
			break;
		}
	}

	// 2) If not found — try match by asset name (may produce multiples)
	if (!Chosen)
	{
		TArray<const FAssetData*> Matches;
		for (const FAssetData& AD : AssetDataList)
		{
			if (AD.AssetName.ToString().Equals(InInteriorSetPathOrName, ESearchCase::IgnoreCase))
			{
				Matches.Add(&AD);
			}
		}
		if (Matches.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: no InteriorSet asset found for '%s'"), *InInteriorSetPathOrName);
			return this;
		}
		if (Matches.Num() > 1)
		{
			UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: multiple InteriorSet assets named '%s', using first match. Consider passing full path to disambiguate."), *InInteriorSetPathOrName);
		}
		Chosen = Matches[0];
	}

	if (!Chosen)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: failed to choose InteriorSet for '%s'"), *InInteriorSetPathOrName);
		return this;
	}

	// Load the asset (if not already loaded)
	UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(Chosen->GetAsset());
	if (!IS)
	{
		IS = Cast<UInteriorSetAsset>(Chosen->ToSoftObjectPath().TryLoad());
	}
	if (!IS)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: failed to load InteriorSet asset '%s'"), *Chosen->ToSoftObjectPath().ToString());
		return this;
	}

	// Find floor by index
	for (const TSoftObjectPtr<UFloorAsset>& FloorRef : IS->Floors)
	{
		UFloorAsset* Floor = FloorRef.LoadSynchronous();
		if (!Floor) continue;
		if (Floor->FloorIndex == InFloorIndex)
		{
			InteriorSetId = IS->InteriorSetID;
			FloorId = Floor->FloorID;
			// also normalize stored path to SoftObjectPath string
			InteriorSetPath = Chosen->ToSoftObjectPath().ToString();
			return this;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: InteriorSet '%s' does not contain floor index %d"), *InInteriorSetPathOrName, InFloorIndex);
#else
	UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::Setup: AssetRegistry not available in this build configuration"));
#endif

	return this;
}

UFloorStatePayload* UFloorStatePayload::SetupFromFloorAsset(UFloorAsset* InFloorAsset)
{
	// Reset
	InteriorSetPath.Empty();
	FloorIndex = -1;
	InteriorSetId.Invalidate();
	FloorId.Invalidate();

	if (!InFloorAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("UFloorStatePayload::SetupFromFloorAsset: InFloorAsset == nullptr"));
		return this;
	}

	// Fill floor GUID and index
	FloorId = InFloorAsset->FloorID;
	FloorIndex = InFloorAsset->FloorIndex;

	// Try to resolve parent interior set GUID via soft ptr
	if (InFloorAsset->ParentInteriorSet.IsValid())
	{
		UInteriorSetAsset* Parent = InFloorAsset->ParentInteriorSet.Get();
		if (!Parent)
		{
			Parent = InFloorAsset->ParentInteriorSet.LoadSynchronous();
		}
		if (Parent)
		{
			InteriorSetId = Parent->InteriorSetID;
			InteriorSetPath = Parent->GetPathName();
		}
	}
	else
	{
#if WITH_EDITOR || WITH_ENGINE || WITH_SERVER_CODE
		// As fallback try to find any InteriorSet that references this floor (rare)
		FString ThisFloorName = InFloorAsset->GetName();
		FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FAssetData> Ads;
		ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), TEXT("InteriorSetAsset")), Ads, true);
		for (const FAssetData& AD : Ads)
		{
			UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
			if (!IS) IS = Cast<UInteriorSetAsset>(AD.ToSoftObjectPath().TryLoad());
			if (!IS) continue;
			for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
			{
				UFloorAsset* Floor = Ref.LoadSynchronous();
				if (!Floor) continue;
				if (Floor == InFloorAsset || Floor->FloorID == InFloorAsset->FloorID)
				{
					InteriorSetId = IS->InteriorSetID;
					InteriorSetPath = AD.ToSoftObjectPath().ToString();
					break;
				}
			}
			if (InteriorSetId.IsValid()) break;
		}
#endif
	}

	// Also store floor asset path for diagnostics
	// (useful in messages / UI)
	// Override InteriorSetPath only if empty (we keep parent path if resolved)
	if (InteriorSetPath.IsEmpty())
	{
		InteriorSetPath = InFloorAsset->GetPathName();
	}

	return this;
}