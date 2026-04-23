#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SComboBox.h"

class IDetailLayoutBuilder;
struct FAssetData;

/** Detail customization for ALocationAnchorActor */
class FLocationAnchorActorDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    // Destination pickers
    void RebuildAnchorList();
    void OnRegionPicked(const FAssetData& AssetData);
    void OnStreetPicked(const FAssetData& AssetData);
    void OnInteriorSetPicked(const FAssetData& AssetData);
    void OnFloorPicked(const FAssetData& AssetData);
    void OnAnchorSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
    FReply OnApplyClicked();

    // Owner pickers
    void OnOwnerRegionPicked(const FAssetData& AssetData);
    void OnOwnerStreetPicked(const FAssetData& AssetData);
    void OnOwnerInteriorSetPicked(const FAssetData& AssetData);
    void OnOwnerFloorPicked(const FAssetData& AssetData);

    // Path getters for Destination
    FString GetSelectedRegionPath() const;
    FString GetSelectedStreetPath() const;
    FString GetSelectedInteriorSetPath() const;
    FString GetSelectedFloorPath() const;

    // Path getters for Owner
    FString GetOwnerRegionPath() const;
    FString GetOwnerStreetPath() const;
    FString GetOwnerInteriorSetPath() const;
    FString GetOwnerFloorPath() const;

    // Asset filter helpers for owner pickers
    bool ShouldFilterStreetAsset(const FAssetData& AssetData) const;
    bool ShouldFilterInteriorSetAsset(const FAssetData& AssetData) const;
    bool ShouldFilterFloorAsset(const FAssetData& AssetData) const;

    // Selected actor(s)
    TArray<TWeakObjectPtr<UObject>> SelectedObjects;

    // Cached pointers
    TWeakObjectPtr<class ALocationAnchorActor> TargetActor;

    // Anchor combo data: entry string contains "DisplayName||GUID"
    TArray<TSharedPtr<FString>> AnchorOptions;
    TSharedPtr<FString> CurrentAnchorSelection;

    // ComboBox reference
    TSharedPtr<SComboBox<TSharedPtr<FString>>> AnchorComboBox;

    // Detail builder pointer for refresh
    IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
};