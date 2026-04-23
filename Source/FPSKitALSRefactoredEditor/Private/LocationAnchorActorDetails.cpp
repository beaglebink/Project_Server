#include "LocationAnchorActorDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SComboBox.h"
#include "PropertyCustomizationHelpers.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "LocationAnchorActor.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"
#include "LocationEditorUtils.h"

#define LOCTEXT_NAMESPACE "FLocationAnchorActorDetails"

template<typename T>
static void ClearSoft(TSoftObjectPtr<T>& Ptr) { Ptr = TSoftObjectPtr<T>(); }

// ── Сканирование уровня на якоря ──────────────────────────────────────────
struct FAnchorOwnerFilter
{
    TSoftObjectPtr<UWorldRegionAsset> Region;
    TSoftObjectPtr<UStreetAsset> Street;
    TSoftObjectPtr<UInteriorSetAsset> InteriorSet;
    TSoftObjectPtr<UFloorAsset> Floor;

    bool IsAny() const
    {
        return !Region.IsNull() || !Street.IsNull() || !InteriorSet.IsNull() || !Floor.IsNull();
    }
};

static void ScanLevelForAnchors(const FSoftObjectPath& LevelSoftPath, TArray<TPair<FGuid, FText>>& OutAnchors, const FAnchorOwnerFilter* OwnerFilter = nullptr, const TSet<FGuid>* AllowedGUIDs = nullptr, const FGuid* ExcludeAnchorID = nullptr)
{
    OutAnchors.Empty();
    if (!LevelSoftPath.IsValid()) return;

    const FString PackageName = LevelSoftPath.GetLongPackageName();
    if (PackageName.IsEmpty()) return;

    UPackage* LevelPackage = FindPackage(nullptr, *PackageName);
    if (!LevelPackage)
        LevelPackage = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!LevelPackage) return;

    UWorld* World = UWorld::FindWorldInPackage(LevelPackage);
    if (!World || !World->PersistentLevel) return;

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        if (ALocationAnchorActor* Anchor = Cast<ALocationAnchorActor>(Actor))
        {
            // Skip the anchor we are currently editing
            if (ExcludeAnchorID && ExcludeAnchorID->IsValid() && Anchor->AnchorID == *ExcludeAnchorID)
                continue;
            if (OwnerFilter && OwnerFilter->IsAny())
            {
                // Debug: show anchor's owner pointers vs filter
                FString FilterRegion = OwnerFilter->Region.IsNull() ? TEXT("<null>") : OwnerFilter->Region.ToSoftObjectPath().GetAssetPathString();
                FString FilterStreet = OwnerFilter->Street.IsNull() ? TEXT("<null>") : OwnerFilter->Street.ToSoftObjectPath().GetAssetPathString();
                FString FilterInterior = OwnerFilter->InteriorSet.IsNull() ? TEXT("<null>") : OwnerFilter->InteriorSet.ToSoftObjectPath().GetAssetPathString();
                FString FilterFloor = OwnerFilter->Floor.IsNull() ? TEXT("<null>") : OwnerFilter->Floor.ToSoftObjectPath().GetAssetPathString();
                FString AnchorRegion = Anchor->OwnerRegion.IsNull() ? TEXT("<null>") : Anchor->OwnerRegion.ToSoftObjectPath().GetAssetPathString();
                FString AnchorStreet = Anchor->OwnerStreet.IsNull() ? TEXT("<null>") : Anchor->OwnerStreet.ToSoftObjectPath().GetAssetPathString();
                FString AnchorInterior = Anchor->OwnerInteriorSet.IsNull() ? TEXT("<null>") : Anchor->OwnerInteriorSet.ToSoftObjectPath().GetAssetPathString();
                FString AnchorFloor = Anchor->OwnerFloor.IsNull() ? TEXT("<null>") : Anchor->OwnerFloor.ToSoftObjectPath().GetAssetPathString();
                UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: Anchor '%s' owners: Region=%s Street=%s Interior=%s Floor=%s  Filter: R=%s S=%s I=%s F=%s"), *Anchor->GetName(), *AnchorRegion, *AnchorStreet, *AnchorInterior, *AnchorFloor, *FilterRegion, *FilterStreet, *FilterInterior, *FilterFloor);
                if (!OwnerFilter->Region.IsNull())
                {
                    if (Anchor->OwnerRegion.IsNull() || Anchor->OwnerRegion.ToSoftObjectPath() != OwnerFilter->Region.ToSoftObjectPath())
                    {
                        UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: skipping '%s' due Region mismatch (anchor owner %s)"), *Anchor->GetName(), *AnchorRegion);
                        continue;
                    }
                }
                if (!OwnerFilter->Street.IsNull())
                {
                    if (Anchor->OwnerStreet.IsNull() || Anchor->OwnerStreet.ToSoftObjectPath() != OwnerFilter->Street.ToSoftObjectPath())
                    {
                        UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: skipping '%s' due Street mismatch (anchor owner %s)"), *Anchor->GetName(), *AnchorStreet);
                        continue;
                    }
                }
                if (!OwnerFilter->InteriorSet.IsNull())
                {
                    if (Anchor->OwnerInteriorSet.IsNull() || Anchor->OwnerInteriorSet.ToSoftObjectPath() != OwnerFilter->InteriorSet.ToSoftObjectPath())
                    {
                        UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: skipping '%s' due InteriorSet mismatch (anchor owner %s)"), *Anchor->GetName(), *AnchorInterior);
                        continue;
                    }
                }
                if (!OwnerFilter->Floor.IsNull())
                {
                    if (Anchor->OwnerFloor.IsNull() || Anchor->OwnerFloor.ToSoftObjectPath() != OwnerFilter->Floor.ToSoftObjectPath())
                    {
                        UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: skipping '%s' due Floor mismatch (anchor owner %s)"), *Anchor->GetName(), *AnchorFloor);
                        continue;
                    }
                }
            }
            FText Label = Anchor->DisplayName.IsEmpty()
                ? FText::FromString(Anchor->GetName()) : Anchor->DisplayName;
            OutAnchors.Add(TPair<FGuid, FText>(Anchor->AnchorID, Label));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: найдено %d якорей"), OutAnchors.Num());
}

// ─────────────────────────────────────────────────────────────────────────────

TSharedRef<IDetailCustomization> FLocationAnchorActorDetails::MakeInstance()
{
    return MakeShareable(new FLocationAnchorActorDetails);
}

void FLocationAnchorActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    CachedDetailBuilder = &DetailBuilder;
    DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

    TargetActor = nullptr;
    for (TWeakObjectPtr<UObject> Obj : SelectedObjects)
    {
        if (ALocationAnchorActor* Actor = Cast<ALocationAnchorActor>(Obj.Get()))
        {
            TargetActor = Actor;
            break;
        }
    }

    // Use a single visible category
    IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(
        "Location Anchor Editor",
        LOCTEXT("CatLabel", "Location Anchor Editor"),
        ECategoryPriority::Important);

    // Owner
    Cat.AddCustomRow(LOCTEXT("OwnerSep", "--- Owner (Registration) ---"))
    .WholeRowContent()
    [
        SNew(STextBlock)
        .Text(LOCTEXT("OwnerSepLabel", "--- Owner (Registration) ---"))
        .Font(IDetailLayoutBuilder::GetDetailFontBold())
    ];

    Cat.AddCustomRow(LOCTEXT("OwnerRegionRow", "Owner Region"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("OwnerRegionLabel", "Owner Region")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UWorldRegionAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetOwnerRegionPath)
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnOwnerRegionPicked)
        .AllowClear(true)
    ];

    Cat.AddCustomRow(LOCTEXT("OwnerStreetRow", "Owner Street"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("OwnerStreetLabel", "Owner Street")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UStreetAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetOwnerStreetPath)
        .OnShouldFilterAsset_Lambda([this](const FAssetData& AD) {
            ALocationAnchorActor* Actor = TargetActor.Get();
            if (!Actor) return false;
            if (Actor->OwnerRegion.IsNull()) return false;
            UStreetAsset* S = Cast<UStreetAsset>(AD.GetAsset());
            if (!S) S = Cast<UStreetAsset>(AD.ToSoftObjectPath().TryLoad());
            if (!S) return true;
            if (S->ParentWorldRegion.IsNull()) return true;
            return S->ParentWorldRegion.ToSoftObjectPath() != Actor->OwnerRegion.ToSoftObjectPath();
        })
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnOwnerStreetPicked)
        .AllowClear(true)
    ];

    Cat.AddCustomRow(LOCTEXT("OwnerInteriorSetRow", "Owner Building"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("OwnerInteriorSetLabel", "Owner Building")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UInteriorSetAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetOwnerInteriorSetPath)
        .OnShouldFilterAsset(this, &FLocationAnchorActorDetails::ShouldFilterInteriorSetAsset)
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnOwnerInteriorSetPicked)
        .AllowClear(true)
    ];

    Cat.AddCustomRow(LOCTEXT("OwnerFloorRow", "Owner Floor"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("OwnerFloorLabel", "Owner Floor")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UFloorAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetOwnerFloorPath)
        .OnShouldFilterAsset_Lambda([this](const FAssetData& AD) {
            ALocationAnchorActor* Actor = TargetActor.Get();
            if (!Actor) return true; // no target actor -> filter out
            // If no interior set selected, hide floors
            if (Actor->OwnerInteriorSet.IsNull()) return true;
            UFloorAsset* F = Cast<UFloorAsset>(AD.GetAsset());
            if (!F) F = Cast<UFloorAsset>(AD.ToSoftObjectPath().TryLoad());
            if (!F) return true;
            if (F->ParentInteriorSet.IsNull()) return true;
            return F->ParentInteriorSet.ToSoftObjectPath() != Actor->OwnerInteriorSet.ToSoftObjectPath();
        })
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnOwnerFloorPicked)
        .AllowClear(true)
    ];

    // Destination
    Cat.AddCustomRow(LOCTEXT("DestSep", "--- Destination (Link) ---"))
    .WholeRowContent()
    [
        SNew(STextBlock)
        .Text(LOCTEXT("DestSepLabel", "--- Destination (Link) ---"))
        .Font(IDetailLayoutBuilder::GetDetailFontBold())
    ];

    Cat.AddCustomRow(LOCTEXT("RegionRow", "Target Region"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("RegionLabel", "Target Region")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UWorldRegionAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetSelectedRegionPath)
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnRegionPicked)
        .AllowClear(true)
    ];

    Cat.AddCustomRow(LOCTEXT("StreetRow", "Target Street"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("StreetLabel", "Target Street")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UStreetAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetSelectedStreetPath)
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnStreetPicked)
        .AllowClear(true)
    ];

    Cat.AddCustomRow(LOCTEXT("InteriorSetRow", "Target Building"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("InteriorSetLabel", "Target Building")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UInteriorSetAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetSelectedInteriorSetPath)
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnInteriorSetPicked)
        .AllowClear(true)
    ];

    Cat.AddCustomRow(LOCTEXT("FloorRow", "Target Floor"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("FloorLabel", "Target Floor")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SObjectPropertyEntryBox)
        .AllowedClass(UFloorAsset::StaticClass())
        .ObjectPath(this, &FLocationAnchorActorDetails::GetSelectedFloorPath)
        .OnObjectChanged(this, &FLocationAnchorActorDetails::OnFloorPicked)
        .AllowClear(true)
    ];

    // Anchor combo + Apply
    RebuildAnchorList();

    Cat.AddCustomRow(LOCTEXT("AnchorRow", "Target Anchor"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("TargetAnchorLabel", "Target Anchor")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f)
        [
            SAssignNew(AnchorComboBox, SComboBox<TSharedPtr<FString>>)
            .OptionsSource(&AnchorOptions)
            .InitiallySelectedItem(CurrentAnchorSelection)
            .OnSelectionChanged(this, &FLocationAnchorActorDetails::OnAnchorSelected)
            .OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
            {
                FString Left, Right;
                const FString Display = (Item.IsValid() && Item->Split(TEXT("||"), &Left, &Right))
                    ? Left : (Item.IsValid() ? *Item : FString());
                return SNew(STextBlock).Text(FText::FromString(Display));
            })
            .Content()
            [
                SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                {
                    if (CurrentAnchorSelection.IsValid())
                    {
                        FString Left, Right;
                        if (CurrentAnchorSelection->Split(TEXT("||"), &Left, &Right))
                            return FText::FromString(Left);
                        return FText::FromString(*CurrentAnchorSelection);
                    }
                    return LOCTEXT("SelectAnchorHint", "-- Select Anchor --");
                })
            ]
        ]
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f)
        [
            SNew(SButton)
            .Text(LOCTEXT("ApplyBtn", "Apply"))
            .OnClicked(this, &FLocationAnchorActorDetails::OnApplyClicked)
        ]
    ];
}

// Owner pickers...

void FLocationAnchorActorDetails::OnOwnerRegionPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->OwnerRegion = TSoftObjectPtr<UWorldRegionAsset>(AssetData.ToSoftObjectPath());
    // Reset child owner fields when region changed
    ClearSoft(Actor->OwnerStreet);
    ClearSoft(Actor->OwnerInteriorSet);
    ClearSoft(Actor->OwnerFloor);
    Actor->MarkPackageDirty();
    if (CachedDetailBuilder) CachedDetailBuilder->ForceRefreshDetails();
}

void FLocationAnchorActorDetails::OnOwnerStreetPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->OwnerStreet = TSoftObjectPtr<UStreetAsset>(AssetData.ToSoftObjectPath());
    // Reset child owner fields when street changed
    ClearSoft(Actor->OwnerInteriorSet);
    ClearSoft(Actor->OwnerFloor);

    if (!Actor->OwnerStreet.IsNull() && Actor->OwnerRegion.IsNull())
    {
        if (UStreetAsset* Street = Actor->OwnerStreet.LoadSynchronous())
            if (!Street->ParentWorldRegion.IsNull())
                Actor->OwnerRegion = Street->ParentWorldRegion;
    }
    Actor->MarkPackageDirty();
    if (CachedDetailBuilder) CachedDetailBuilder->ForceRefreshDetails();
}

void FLocationAnchorActorDetails::OnOwnerInteriorSetPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->OwnerInteriorSet = TSoftObjectPtr<UInteriorSetAsset>(AssetData.ToSoftObjectPath());
    // Reset child owner fields when interior set changed
    ClearSoft(Actor->OwnerFloor);

    if (!Actor->OwnerInteriorSet.IsNull())
    {
        if (UInteriorSetAsset* Interior = Actor->OwnerInteriorSet.LoadSynchronous())
        {
            if (Actor->OwnerStreet.IsNull() && !Interior->ParentStreet.IsNull())
                Actor->OwnerStreet = Interior->ParentStreet;
            if (Actor->OwnerRegion.IsNull() && !Actor->OwnerStreet.IsNull())
            {
                if (UStreetAsset* Street = Actor->OwnerStreet.LoadSynchronous())
                    if (!Street->ParentWorldRegion.IsNull())
                        Actor->OwnerRegion = Street->ParentWorldRegion;
            }
        }
    }
    Actor->MarkPackageDirty();
    if (CachedDetailBuilder) CachedDetailBuilder->ForceRefreshDetails();
}

void FLocationAnchorActorDetails::OnOwnerFloorPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->OwnerFloor = TSoftObjectPtr<UFloorAsset>(AssetData.ToSoftObjectPath());
    Actor->OwnerContextType = ELocationContextType::Floor;
    if (!Actor->OwnerFloor.IsNull())
    {
        if (UFloorAsset* Floor = Actor->OwnerFloor.LoadSynchronous())
        {
            if (Actor->OwnerInteriorSet.IsNull() && !Floor->ParentInteriorSet.IsNull())
                Actor->OwnerInteriorSet = Floor->ParentInteriorSet;
            if (!Actor->OwnerInteriorSet.IsNull() && Actor->OwnerStreet.IsNull())
            {
                if (UInteriorSetAsset* Interior = Actor->OwnerInteriorSet.LoadSynchronous())
                    if (!Interior->ParentStreet.IsNull())
                        Actor->OwnerStreet = Interior->ParentStreet;
            }
            if (!Actor->OwnerStreet.IsNull() && Actor->OwnerRegion.IsNull())
            {
                if (UStreetAsset* Street = Actor->OwnerStreet.LoadSynchronous())
                    if (!Street->ParentWorldRegion.IsNull())
                        Actor->OwnerRegion = Street->ParentWorldRegion;
            }
        }
    }
    Actor->MarkPackageDirty();
    if (CachedDetailBuilder) CachedDetailBuilder->ForceRefreshDetails();
}

// Destination pickers implementations...

void FLocationAnchorActorDetails::OnRegionPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->DestinationLink.TargetRegion = TSoftObjectPtr<UWorldRegionAsset>(AssetData.ToSoftObjectPath());
    ClearSoft(Actor->DestinationLink.TargetStreet);
    ClearSoft(Actor->DestinationLink.TargetInteriorSet);
    ClearSoft(Actor->DestinationLink.TargetFloor);
    Actor->DestinationLink.TargetAnchorID          = FGuid();
    Actor->DestinationLink.TargetAnchorDisplayName = FText::GetEmpty();
    RebuildAnchorList();
    if (AnchorComboBox.IsValid()) AnchorComboBox->RefreshOptions();
}

void FLocationAnchorActorDetails::OnStreetPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->DestinationLink.TargetStreet = TSoftObjectPtr<UStreetAsset>(AssetData.ToSoftObjectPath());
    ClearSoft(Actor->DestinationLink.TargetInteriorSet);
    ClearSoft(Actor->DestinationLink.TargetFloor);
    Actor->DestinationLink.TargetAnchorID          = FGuid();
    Actor->DestinationLink.TargetAnchorDisplayName = FText::GetEmpty();
    RebuildAnchorList();
    if (AnchorComboBox.IsValid()) AnchorComboBox->RefreshOptions();
}

void FLocationAnchorActorDetails::OnInteriorSetPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->DestinationLink.TargetInteriorSet = TSoftObjectPtr<UInteriorSetAsset>(AssetData.ToSoftObjectPath());
    ClearSoft(Actor->DestinationLink.TargetFloor);
    Actor->DestinationLink.TargetAnchorID          = FGuid();
    Actor->DestinationLink.TargetAnchorDisplayName = FText::GetEmpty();
    RebuildAnchorList();
    if (AnchorComboBox.IsValid()) AnchorComboBox->RefreshOptions();
}

void FLocationAnchorActorDetails::OnFloorPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->DestinationLink.TargetFloor = TSoftObjectPtr<UFloorAsset>(AssetData.ToSoftObjectPath());
    Actor->DestinationLink.TargetAnchorID          = FGuid();
    Actor->DestinationLink.TargetAnchorDisplayName = FText::GetEmpty();
    RebuildAnchorList();
    if (AnchorComboBox.IsValid()) AnchorComboBox->RefreshOptions();
}

// Anchor list / selection...

void FLocationAnchorActorDetails::RebuildAnchorList()
{
    AnchorOptions.Empty();
    CurrentAnchorSelection.Reset();

    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;

    FSoftObjectPath LevelPath;

    // Priority: Region -> Street -> InteriorSet (Building) -> Floor
    if (!Actor->DestinationLink.TargetRegion.IsNull())
    {
        UWorldRegionAsset* Region = Actor->DestinationLink.TargetRegion.LoadSynchronous();
        if (Region && !Region->RegionLevel.IsNull())
        {
            LevelPath = Region->RegionLevel.ToSoftObjectPath();
        }
    }

    if (!LevelPath.IsValid() && !Actor->DestinationLink.TargetStreet.IsNull())
    {
        UStreetAsset* Street = Actor->DestinationLink.TargetStreet.LoadSynchronous();
        if (Street && !Street->ParentWorldRegion.IsNull())
        {
            if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous())
            {
                if (!R->RegionLevel.IsNull())
                    LevelPath = R->RegionLevel.ToSoftObjectPath();
            }
        }
    }

    if (!LevelPath.IsValid() && !Actor->DestinationLink.TargetInteriorSet.IsNull())
    {
        UInteriorSetAsset* IS = Actor->DestinationLink.TargetInteriorSet.LoadSynchronous();
        if (IS)
        {
            // Prefer first floor's level if available
            if (!IS->Floors.IsEmpty())
            {
                UFloorAsset* FF = IS->Floors[0].LoadSynchronous();
                if (FF && !FF->FloorLevel.IsNull())
                {
                    LevelPath = FF->FloorLevel.ToSoftObjectPath();
                }
            }

            // Fallback to parent street -> parent region level
            if (!LevelPath.IsValid() && !IS->ParentStreet.IsNull())
            {
                if (UStreetAsset* S = IS->ParentStreet.LoadSynchronous())
                {
                    if (!S->ParentWorldRegion.IsNull())
                    {
                        if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous())
                        {
                            if (!R->RegionLevel.IsNull())
                                LevelPath = R->RegionLevel.ToSoftObjectPath();
                        }
                    }
                }
            }
        }
    }

    if (!LevelPath.IsValid() && !Actor->DestinationLink.TargetFloor.IsNull())
    {
        UFloorAsset* Floor = Actor->DestinationLink.TargetFloor.LoadSynchronous();
        if (Floor && !Floor->FloorLevel.IsNull())
            LevelPath = Floor->FloorLevel.ToSoftObjectPath();
    }

    // If a specific floor is selected, prefer its level regardless of previously resolved LevelPath
    if (!Actor->DestinationLink.TargetFloor.IsNull())
    {
        if (UFloorAsset* Floor = Actor->DestinationLink.TargetFloor.LoadSynchronous())
        {
            if (!Floor->FloorLevel.IsNull())
            {
                LevelPath = Floor->FloorLevel.ToSoftObjectPath();
            }
        }
    }

    // Debug logging: show which destination pointers are set and resolved LevelPath
    {
        FString RegPath = Actor->DestinationLink.TargetRegion.IsNull() ? TEXT("<null>") : Actor->DestinationLink.TargetRegion.ToSoftObjectPath().GetAssetPathString();
        FString StrPath = Actor->DestinationLink.TargetStreet.IsNull() ? TEXT("<null>") : Actor->DestinationLink.TargetStreet.ToSoftObjectPath().GetAssetPathString();
        FString IntPath = Actor->DestinationLink.TargetInteriorSet.IsNull() ? TEXT("<null>") : Actor->DestinationLink.TargetInteriorSet.ToSoftObjectPath().GetAssetPathString();
        FString FlrPath = Actor->DestinationLink.TargetFloor.IsNull() ? TEXT("<null>") : Actor->DestinationLink.TargetFloor.ToSoftObjectPath().GetAssetPathString();
        FString Lvl = LevelPath.IsValid() ? LevelPath.ToString() : TEXT("<invalid>");
        UE_LOG(LogTemp, Log, TEXT("RebuildAnchorList: Region=%s Street=%s InteriorSet=%s Floor=%s ResolvedLevel=%s"), *RegPath, *StrPath, *IntPath, *FlrPath, *Lvl);
    }

    TArray<TPair<FGuid, FText>> FoundAnchors2;
    FAnchorOwnerFilter OwnerFilter;
    if (!Actor->DestinationLink.TargetRegion.IsNull()) OwnerFilter.Region = Actor->DestinationLink.TargetRegion;
    if (!Actor->DestinationLink.TargetStreet.IsNull()) OwnerFilter.Street = Actor->DestinationLink.TargetStreet;
    if (!Actor->DestinationLink.TargetInteriorSet.IsNull()) OwnerFilter.InteriorSet = Actor->DestinationLink.TargetInteriorSet;
    if (!Actor->DestinationLink.TargetFloor.IsNull()) OwnerFilter.Floor = Actor->DestinationLink.TargetFloor;

    // If LevelPath is valid, scan that level. Otherwise scan all loaded worlds' persistent levels.
    if (LevelPath.IsValid())
    {
        const FGuid* Exclude = Actor ? &Actor->AnchorID : nullptr;
        ScanLevelForAnchors(LevelPath, FoundAnchors2, &OwnerFilter, nullptr, Exclude);
    }
    else
    {
        if (GEngine)
        {
            const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
            for (const FWorldContext& WC : WorldContexts)
            {
                UWorld* World = WC.World();
                if (!World || !World->PersistentLevel) continue;
                FSoftObjectPath SoftPath(World->PersistentLevel->GetOutermost()->GetPathName());
                TArray<TPair<FGuid, FText>> Temp;
                const FGuid* Exclude = Actor ? &Actor->AnchorID : nullptr;
                ScanLevelForAnchors(SoftPath, Temp, &OwnerFilter, nullptr, Exclude);
                for (const auto& P : Temp) FoundAnchors2.Add(P);
            }
        }
    }

    TArray<TPair<FGuid, FText>>& FoundAnchors = FoundAnchors2;

    for (const auto& P : FoundAnchors)
    {
        FString Entry = FString::Printf(TEXT("%s||%s"), *P.Value.ToString(), *P.Key.ToString());
        AnchorOptions.Add(MakeShared<FString>(Entry));
    }

    if (AnchorOptions.Num() == 0)
    {
        AnchorOptions.Add(MakeShared<FString>(LevelPath.IsValid() ? TEXT("<Якоря не найдены>") : TEXT("<Укажите Floor или Region>")));
        CurrentAnchorSelection = AnchorOptions[0];
        return;
    }

    const FGuid& SavedGuid = Actor->DestinationLink.TargetAnchorID;
    if (SavedGuid.IsValid())
    {
        for (auto& Item : AnchorOptions)
        {
            FString Left, Right;
            if (Item->Split(TEXT("||"), &Left, &Right))
            {
                FGuid G;
                if (FGuid::Parse(Right, G) && G == SavedGuid)
                {
                    CurrentAnchorSelection = Item;
                    break;
                }
            }
        }
    }

    if (!CurrentAnchorSelection.IsValid())
        CurrentAnchorSelection = AnchorOptions[0];
}

void FLocationAnchorActorDetails::OnAnchorSelected(TSharedPtr<FString> NewValue, ESelectInfo::Type)
{
    CurrentAnchorSelection = NewValue;
}

FReply FLocationAnchorActorDetails::OnApplyClicked()
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return FReply::Handled();

    // Разбираем выбранный якорь
    FGuid SelectedGuid;
    FText SelectedDisplayName;
    if (CurrentAnchorSelection.IsValid())
    {
        FString Left, Right;
        if (CurrentAnchorSelection->Split(TEXT("||"), &Left, &Right))
        {
            FGuid::Parse(Right, SelectedGuid);
            SelectedDisplayName = FText::FromString(Left);
        }
    }

    if (!SelectedGuid.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("LocationAnchorActorDetails: якорь не выбран"));
        return FReply::Handled();
    }

    // Сохраняем прямую ссылку
    Actor->Modify();
    Actor->DestinationLink.TargetAnchorID          = SelectedGuid;
    Actor->DestinationLink.TargetAnchorDisplayName = SelectedDisplayName;
    Actor->MarkPackageDirty();

    // ── Миграция: удаляем старую запись TransitionPoint из ВСЕХ ассетов ──
    // (на случай если Owner актора изменился с прошлого Apply)
    if (Actor->AnchorID.IsValid())
    {
        const int32 Removed = ULocationEditorUtils::RemoveTransitionPointFromAllAssets(Actor->AnchorID);
        if (Removed > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("LocationAnchorActorDetails: удалено %d старых TransitionPoint записей для '%s'"),
                Removed, *Actor->GetName());
        }
    }

    // Собираем адрес источника из Owner-полей актора
    TSoftObjectPtr<UWorldRegionAsset> SourceRegion      = Actor->OwnerRegion;
    TSoftObjectPtr<UStreetAsset>      SourceStreet      = Actor->OwnerStreet;
    TSoftObjectPtr<UInteriorSetAsset> SourceInteriorSet = Actor->OwnerInteriorSet;
    TSoftObjectPtr<UFloorAsset>       SourceFloor       = Actor->OwnerFloor;

    // Определяем уровень целевого якоря
    FSoftObjectPath TargetLevelPath;
    if (!Actor->DestinationLink.TargetFloor.IsNull())
    {
        if (UFloorAsset* Floor = Actor->DestinationLink.TargetFloor.LoadSynchronous())
            if (!Floor->FloorLevel.IsNull())
                TargetLevelPath = Floor->FloorLevel.ToSoftObjectPath();
    }
    if (!TargetLevelPath.IsValid() && !Actor->DestinationLink.TargetRegion.IsNull())
    {
        if (UWorldRegionAsset* Region = Actor->DestinationLink.TargetRegion.LoadSynchronous())
            if (!Region->RegionLevel.IsNull())
                TargetLevelPath = Region->RegionLevel.ToSoftObjectPath();
    }

    // Записываем обратную ссылку в целевом якоре и регистрируем его TransitionPoint
    if (TargetLevelPath.IsValid())
    {
        const FString PackageName = TargetLevelPath.GetLongPackageName();
        UPackage* LevelPackage = FindPackage(nullptr, *PackageName);
        if (!LevelPackage)
            LevelPackage = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);

        if (LevelPackage)
        {
            UWorld* TargetWorld = UWorld::FindWorldInPackage(LevelPackage);
            if (TargetWorld && TargetWorld->PersistentLevel)
            {
                for (AActor* LevelActor : TargetWorld->PersistentLevel->Actors)
                {
                    if (!IsValid(LevelActor)) continue;
                    ALocationAnchorActor* DestAnchor = Cast<ALocationAnchorActor>(LevelActor);
                    if (!DestAnchor || DestAnchor->AnchorID != SelectedGuid) continue;

                    DestAnchor->Modify();
                    DestAnchor->DestinationLink.TargetRegion      = SourceRegion;
                    DestAnchor->DestinationLink.TargetStreet      = SourceStreet;
                    DestAnchor->DestinationLink.TargetInteriorSet = SourceInteriorSet;
                    DestAnchor->DestinationLink.TargetFloor       = SourceFloor;
                    DestAnchor->DestinationLink.TargetAnchorID    = Actor->AnchorID;
                    DestAnchor->DestinationLink.TargetAnchorDisplayName = Actor->DisplayName.IsEmpty()
                        ? FText::FromString(Actor->GetName()) : Actor->DisplayName;

                    // Зарегистрировать переходной пункт для целевого якоря
                    ULocationEditorUtils::RegisterTransitionPoint(DestAnchor);

                    LevelPackage->MarkPackageDirty();

                    UE_LOG(LogTemp, Log, TEXT("LocationAnchorActorDetails: обратная ссылка записана в '%s' [%s]"),
                        *DestAnchor->GetName(), *DestAnchor->AnchorID.ToString());
                    break;
                }
            }
        }
    }

    // Регистрируем TransitionPoint в ассете источника
    ULocationEditorUtils::RegisterTransitionPoint(Actor);

    if (CachedDetailBuilder) CachedDetailBuilder->ForceRefreshDetails();
    return FReply::Handled();
}

// Path getters...

FString FLocationAnchorActorDetails::GetSelectedRegionPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->DestinationLink.TargetRegion.IsNull()) return FString();
    return Actor->DestinationLink.TargetRegion.ToSoftObjectPath().GetAssetPathString();
}

FString FLocationAnchorActorDetails::GetSelectedStreetPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->DestinationLink.TargetStreet.IsNull()) return FString();
    return Actor->DestinationLink.TargetStreet.ToSoftObjectPath().GetAssetPathString();
}

FString FLocationAnchorActorDetails::GetSelectedInteriorSetPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->DestinationLink.TargetInteriorSet.IsNull()) return FString();
    return Actor->DestinationLink.TargetInteriorSet.ToSoftObjectPath().GetAssetPathString();
}

FString FLocationAnchorActorDetails::GetSelectedFloorPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->DestinationLink.TargetFloor.IsNull()) return FString();
    return Actor->DestinationLink.TargetFloor.ToSoftObjectPath().GetAssetPathString();
}

FString FLocationAnchorActorDetails::GetOwnerRegionPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->OwnerRegion.IsNull()) return FString();
    return Actor->OwnerRegion.ToSoftObjectPath().GetAssetPathString();
}

FString FLocationAnchorActorDetails::GetOwnerStreetPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->OwnerStreet.IsNull()) return FString();
    return Actor->OwnerStreet.ToSoftObjectPath().GetAssetPathString();
}

bool FLocationAnchorActorDetails::ShouldFilterStreetAsset(const FAssetData& AssetData) const
{
    // Show only streets that belong to selected region (if any)
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return false;
    if (Actor->OwnerRegion.IsNull()) return false;

    UStreetAsset* S = Cast<UStreetAsset>(AssetData.GetAsset());
    if (!S) S = Cast<UStreetAsset>(AssetData.ToSoftObjectPath().TryLoad());
    if (!S) return true; // filter out invalid

    if (S->ParentWorldRegion.IsNull()) return true;
    return S->ParentWorldRegion.ToSoftObjectPath() != Actor->OwnerRegion.ToSoftObjectPath();
}

FString FLocationAnchorActorDetails::GetOwnerInteriorSetPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->OwnerInteriorSet.IsNull()) return FString();
    return Actor->OwnerInteriorSet.ToSoftObjectPath().GetAssetPathString();
}

bool FLocationAnchorActorDetails::ShouldFilterInteriorSetAsset(const FAssetData& AssetData) const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return false;
    if (Actor->OwnerStreet.IsNull()) return false;

    UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AssetData.GetAsset());
    if (!IS) IS = Cast<UInteriorSetAsset>(AssetData.ToSoftObjectPath().TryLoad());
    if (!IS) return true;

    if (IS->ParentStreet.IsNull()) return true;
    return IS->ParentStreet.ToSoftObjectPath() != Actor->OwnerStreet.ToSoftObjectPath();
}

FString FLocationAnchorActorDetails::GetOwnerFloorPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->OwnerFloor.IsNull()) return FString();
    return Actor->OwnerFloor.ToSoftObjectPath().GetAssetPathString();
}

bool FLocationAnchorActorDetails::ShouldFilterFloorAsset(const FAssetData& AssetData) const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    // If no actor or no interior set selected, hide (filter out) floors
    if (!Actor) return true;
    if (Actor->OwnerInteriorSet.IsNull()) return true;

    UFloorAsset* F = Cast<UFloorAsset>(AssetData.GetAsset());
    if (!F) F = Cast<UFloorAsset>(AssetData.ToSoftObjectPath().TryLoad());
    if (!F) return true;

    if (F->ParentInteriorSet.IsNull()) return true;
    return F->ParentInteriorSet.ToSoftObjectPath() != Actor->OwnerInteriorSet.ToSoftObjectPath();
}

#undef LOCTEXT_NAMESPACE