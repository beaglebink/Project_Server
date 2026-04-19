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

// Вспомогательная функция: очистить TSoftObjectPtr полностью (путь + объект)
template<typename T>
static void ClearSoft(TSoftObjectPtr<T>& Ptr) { Ptr = TSoftObjectPtr<T>(); }

// ── Сканирование уровня без открытия в редакторе ──────────────────────────
static void ScanLevelForAnchors(
    const FSoftObjectPath& LevelSoftPath,
    TArray<TPair<FGuid, FText>>& OutAnchors)
{
    OutAnchors.Empty();

    if (!LevelSoftPath.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ScanLevelForAnchors: путь невалиден"));
        return;
    }

    const FString PackageName = LevelSoftPath.GetLongPackageName();
    UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: пакет = '%s'"), *PackageName);

    if (PackageName.IsEmpty()) return;

    UPackage* LevelPackage = FindPackage(nullptr, *PackageName);
    if (!LevelPackage)
        LevelPackage = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);

    if (!LevelPackage)
    {
        UE_LOG(LogTemp, Warning, TEXT("ScanLevelForAnchors: не удалось загрузить пакет '%s'"), *PackageName);
        return;
    }

    UWorld* World = UWorld::FindWorldInPackage(LevelPackage);
    if (!World || !World->PersistentLevel)
    {
        UE_LOG(LogTemp, Warning, TEXT("ScanLevelForAnchors: UWorld или PersistentLevel не найден"));
        return;
    }

    UE_LOG(LogTemp, Log,
        TEXT("ScanLevelForAnchors: акторов = %d"), World->PersistentLevel->Actors.Num());

    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        if (ALocationAnchorActor* Anchor = Cast<ALocationAnchorActor>(Actor))
        {
            // Запасной label: если DisplayName пустой, используем имя актора
            FText Label = Anchor->DisplayName.IsEmpty() ? FText::FromString(Anchor->GetName()) : Anchor->DisplayName;

            UE_LOG(LogTemp, Log,
                TEXT("ScanLevelForAnchors: якорь '%s' [%s]"),
                *Label.ToString(),
                *Anchor->AnchorID.ToString());

            OutAnchors.Add(TPair<FGuid, FText>(Anchor->AnchorID, Label));
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ScanLevelForAnchors: найдено якорей = %d"), OutAnchors.Num());
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

    {
        TSharedRef<IPropertyHandle> DestHandle = DetailBuilder.GetProperty(
            GET_MEMBER_NAME_CHECKED(ALocationAnchorActor, DestinationLink));
        DetailBuilder.HideProperty(DestHandle);
    }

    IDetailCategoryBuilder& Cat = DetailBuilder.EditCategory(
        "Location Anchor (Editor)", FText::GetEmpty(), ECategoryPriority::Important);

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

    // Заполняем список до создания виджета
    RebuildAnchorList();

    Cat.AddCustomRow(LOCTEXT("AnchorRow", "Target Anchor"))
    .NameContent()[ SNew(STextBlock).Text(LOCTEXT("AnchorLabel", "Target Anchor")) ]
    .ValueContent().MaxDesiredWidth(600.f)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.f)
        [
            // Сохраняем ссылку на ComboBox
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
        + SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
        [
            SNew(SButton)
            .Text(LOCTEXT("ApplyBtn", "Apply"))
            .OnClicked(this, &FLocationAnchorActorDetails::OnApplyClicked)
        ]
    ];
}

// ── Pickers: НЕ вызываем ForceRefreshDetails — только обновляем список якорей ──

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

// ── Anchor list ───────────────────────────────────────────────────────────

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
        FString Entry = FString::Printf(TEXT("%s||%s"),
            *P.Value.ToString(), *P.Key.ToString());
        AnchorOptions.Add(MakeShared<FString>(Entry));
    }

    if (AnchorOptions.Num() == 0)
    {
        FString Hint = LevelPath.IsValid()
            ? TEXT("<Якоря не найдены на уровне>")
            : TEXT("<Укажите Floor или Region с уровнем>");
        AnchorOptions.Add(MakeShared<FString>(Hint));
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

// ── Apply: прямая ссылка + обратная связь ─────────────────────────────────

FReply FLocationAnchorActorDetails::OnApplyClicked()
{
    ALocationAnchorActor* Actor = TargetActor.Get();
    if (!Actor) return FReply::Handled();

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

    // Записываем прямую ссылку
    Actor->Modify();
    Actor->DestinationLink.TargetAnchorID          = SelectedGuid;
    Actor->DestinationLink.TargetAnchorDisplayName = SelectedDisplayName;
    Actor->MarkPackageDirty();

    // Определяем уровень, на котором находится целевой якорь
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

    // Определяем уровень, на котором находится ТЕКУЩИЙ редактируемый актор (для обратной ссылки)
    FSoftObjectPath SourceLevelPath;
    if (Actor->OwnerContextType == ELocationContextType::Floor && !Actor->OwnerFloor.IsNull())
    {
        if (UFloorAsset* OwnerFloor = Actor->OwnerFloor.LoadSynchronous())
            if (!OwnerFloor->FloorLevel.IsNull())
                SourceLevelPath = OwnerFloor->FloorLevel.ToSoftObjectPath();
    }
    else if (Actor->OwnerContextType == ELocationContextType::Street && !Actor->OwnerRegion.IsNull())
    {
        if (UWorldRegionAsset* OwnerRegion = Actor->OwnerRegion.LoadSynchronous())
            if (!OwnerRegion->RegionLevel.IsNull())
                SourceLevelPath = OwnerRegion->RegionLevel.ToSoftObjectPath();
    }

    // Записываем обратную ссылку в целевом якоре
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
                    DestAnchor->DestinationLink.TargetAnchorID          = Actor->AnchorID;
                    DestAnchor->DestinationLink.TargetAnchorDisplayName = Actor->DisplayName.IsEmpty()
                        ? FText::FromString(Actor->GetName())
                        : Actor->DisplayName;

                    ClearSoft(DestAnchor->DestinationLink.TargetFloor);
                    ClearSoft(DestAnchor->DestinationLink.TargetRegion);
                    ClearSoft(DestAnchor->DestinationLink.TargetStreet);
                    ClearSoft(DestAnchor->DestinationLink.TargetInteriorSet);

                    if (Actor->OwnerContextType == ELocationContextType::Floor)
                        DestAnchor->DestinationLink.TargetFloor = Actor->OwnerFloor;
                    else if (Actor->OwnerContextType == ELocationContextType::Street)
                        DestAnchor->DestinationLink.TargetRegion = Actor->OwnerRegion;

                    LevelPackage->MarkPackageDirty();

                    UE_LOG(LogTemp, Log,
                        TEXT("LocationAnchorActorDetails: обратная ссылка → '%s' [%s]"),
                        *DestAnchor->DisplayName.ToString(), *DestAnchor->AnchorID.ToString());
                    break;
                }
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("LocationAnchorActorDetails: Apply '%s'"), *SelectedDisplayName.ToString());

    // Пересобираем детали чтобы отражать изменения после Apply
    if (CachedDetailBuilder) CachedDetailBuilder->ForceRefreshDetails();
    return FReply::Handled();
}

// ── Path getters ──────────────────────────────────────────────────────────

FString FLocationAnchorActorDetails::GetSelectedRegionPath() const
{
    if (ALocationAnchorActor* Actor = TargetActor.Get())
        if (!Actor->DestinationLink.TargetRegion.IsNull())
            return Actor->DestinationLink.TargetRegion.ToSoftObjectPath().GetAssetPathString();
    return FString();
}

FString FLocationAnchorActorDetails::GetSelectedStreetPath() const
{
    if (ALocationAnchorActor* Actor = TargetActor.Get())
        if (!Actor->DestinationLink.TargetStreet.IsNull())
            return Actor->DestinationLink.TargetStreet.ToSoftObjectPath().GetAssetPathString();
    return FString();
}

FString FLocationAnchorActorDetails::GetSelectedInteriorSetPath() const
{
    if (ALocationAnchorActor* Actor = TargetActor.Get())
        if (!Actor->DestinationLink.TargetInteriorSet.IsNull())
            return Actor->DestinationLink.TargetInteriorSet.ToSoftObjectPath().GetAssetPathString();
    return FString();
}

FString FLocationAnchorActorDetails::GetSelectedFloorPath() const
{
    if (ALocationAnchorActor* Actor = TargetActor.Get())
        if (!Actor->DestinationLink.TargetFloor.IsNull())
            return Actor->DestinationLink.TargetFloor.ToSoftObjectPath().GetAssetPathString();
    return FString();
}

#undef LOCTEXT_NAMESPACE