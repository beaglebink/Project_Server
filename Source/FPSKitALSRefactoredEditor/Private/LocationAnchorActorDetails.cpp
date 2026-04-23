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
static void ScanLevelForAnchors(const FSoftObjectPath& LevelSoftPath, TArray<TPair<FGuid, FText>>& OutAnchors)
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
    Actor->MarkPackageDirty();
}

void FLocationAnchorActorDetails::OnOwnerStreetPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->OwnerStreet = TSoftObjectPtr<UStreetAsset>(AssetData.ToSoftObjectPath());
    if (!Actor->OwnerStreet.IsNull() && Actor->OwnerRegion.IsNull())
    {
        if (UStreetAsset* Street = Actor->OwnerStreet.LoadSynchronous())
            if (!Street->ParentWorldRegion.IsNull())
                Actor->OwnerRegion = Street->ParentWorldRegion;
    }
    Actor->MarkPackageDirty();
}

void FLocationAnchorActorDetails::OnOwnerInteriorSetPicked(const FAssetData& AssetData)
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return;
    Actor->Modify();
    Actor->OwnerInteriorSet = TSoftObjectPtr<UInteriorSetAsset>(AssetData.ToSoftObjectPath());
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

    if (!Actor->DestinationLink.TargetFloor.IsNull())
    {
        UFloorAsset* Floor = Actor->DestinationLink.TargetFloor.LoadSynchronous();
        if (Floor && !Floor->FloorLevel.IsNull())
            LevelPath = Floor->FloorLevel.ToSoftObjectPath();
    }

    if (!LevelPath.IsValid() && !Actor->DestinationLink.TargetRegion.IsNull())
    {
        UWorldRegionAsset* Region = Actor->DestinationLink.TargetRegion.LoadSynchronous();
        if (Region && !Region->RegionLevel.IsNull())
            LevelPath = Region->RegionLevel.ToSoftObjectPath();
    }

    TArray<TPair<FGuid, FText>> FoundAnchors;
    ScanLevelForAnchors(LevelPath, FoundAnchors);

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

FString FLocationAnchorActorDetails::GetOwnerInteriorSetPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->OwnerInteriorSet.IsNull()) return FString();
    return Actor->OwnerInteriorSet.ToSoftObjectPath().GetAssetPathString();
}

FString FLocationAnchorActorDetails::GetOwnerFloorPath() const
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor || Actor->OwnerFloor.IsNull()) return FString();
    return Actor->OwnerFloor.ToSoftObjectPath().GetAssetPathString();
}

#undef LOCTEXT_NAMESPACE