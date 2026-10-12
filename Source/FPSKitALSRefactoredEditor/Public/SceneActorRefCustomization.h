#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "SceneActorRef.h"

class IPropertyHandle;
template<typename T> class SComboBox;

class FSceneActorRefCustomization : public IPropertyTypeCustomization
{
public:
    static TSharedRef<IPropertyTypeCustomization> MakeInstance();

    virtual void CustomizeHeader(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        FDetailWidgetRow& HeaderRow,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

    virtual void CustomizeChildren(
        TSharedRef<IPropertyHandle> StructPropertyHandle,
        IDetailChildrenBuilder& ChildBuilder,
        IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:
    void OnSceneChanged();
    void OnSceneKindChanged();
    void RebuildActorList();
    void OnActorSelected(TSharedPtr<FGuid> NewSelection, ESelectInfo::Type SelectInfo);
    TSharedRef<SWidget> MakeActorComboItem(TSharedPtr<FGuid> Item);
    FText GetCurrentActorLabel() const;

    // --- Handles ---
    TSharedPtr<IPropertyHandle> SceneKindHandle;
    TSharedPtr<IPropertyHandle> SceneFloorHandle;
    TSharedPtr<IPropertyHandle> SceneRegionHandle;
    TSharedPtr<IPropertyHandle> ActorPersistentIdHandle;
    TSharedPtr<IPropertyHandle> ActorLabelHandle;

    // --- Utils ---
    TSharedPtr<IPropertyUtilities> PropertyUtilities;

    // --- Helpers ---
    ESceneAssetKind GetSceneKindFromHandle() const;
    UWorld* ResolveWorldFromSceneSelection() const;

    FGuid GetActorGuidFromHandle() const;
    void  SetActorGuidOnHandle(const FGuid& NewGuid) const;

    TArray<TSharedPtr<FGuid>> ActorOptions;
    TMap<FGuid, FString>      ActorLabels;
    TSharedPtr<SComboBox<TSharedPtr<FGuid>>> ActorComboBox;
};