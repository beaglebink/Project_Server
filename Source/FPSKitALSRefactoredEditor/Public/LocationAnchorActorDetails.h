#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Templates/SharedPointer.h"
#include "Widgets/Input/SComboBox.h"

class IDetailLayoutBuilder;

/** Detail customization for ALocationAnchorActor */
class FLocationAnchorActorDetails : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    // Helpers
    void RebuildAnchorList();
    void OnRegionPicked(const FAssetData& AssetData);
    void OnStreetPicked(const FAssetData& AssetData);
    void OnInteriorSetPicked(const FAssetData& AssetData);
    void OnFloorPicked(const FAssetData& AssetData);
    void OnAnchorSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
    FReply OnApplyClicked();

    // Helpers used by SObjectPropertyEntryBox to show current picked asset path
    FString GetSelectedRegionPath() const;
    FString GetSelectedStreetPath() const;
    FString GetSelectedInteriorSetPath() const;
    FString GetSelectedFloorPath() const;

    // Selected actor(s)
    TArray<TWeakObjectPtr<UObject>> SelectedObjects;

    // Cached pointers
    TWeakObjectPtr<class ALocationAnchorActor> TargetActor;

    // Anchor combo data: entry string contains "DisplayName||GUID"
    TArray<TSharedPtr<FString>> AnchorOptions;
    TSharedPtr<FString> CurrentAnchorSelection;

    // —сылка на ComboBox дл€ обновлени€ без ForceRefreshDetails
    TSharedPtr<SComboBox<TSharedPtr<FString>>> AnchorComboBox;

    // Detail builder pointer for refresh
    IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
};