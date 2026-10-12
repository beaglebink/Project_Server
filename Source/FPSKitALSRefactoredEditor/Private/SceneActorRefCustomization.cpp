#include "SceneActorRefCustomization.h"

#include "SceneActorRef.h"
#include "FloorAsset.h"
#include "WorldRegionAsset.h"
#include "InteractiveItemComponent.h"
#include "FloorAssignmentComponent.h"

#include "IDetailChildrenBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"

#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"
#include "IPropertyUtilities.h"

#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"

#include "Engine/World.h"
#include "Engine/Level.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "SceneActorRefCustomization"

TSharedRef<IPropertyTypeCustomization> FSceneActorRefCustomization::MakeInstance()
{
    return MakeShared<FSceneActorRefCustomization>();
}

void FSceneActorRefCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& /*CustomizationUtils*/)
{
    HeaderRow
        .NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        [
            SNew(STextBlock)
                .Text(LOCTEXT("Summary", "(scene + actor)"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
        ];
}

void FSceneActorRefCustomization::CustomizeChildren(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& CustomizationUtils)
{
    PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

    SceneKindHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, SceneKind));
    SceneFloorHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, SceneFloor));
    SceneRegionHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, SceneRegion));
    ActorPersistentIdHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, ActorPersistentId));
    ActorLabelHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, ActorLabel));

    // --- SceneKind ---
    if (SceneKindHandle.IsValid())
    {
        ChildBuilder.AddProperty(SceneKindHandle.ToSharedRef())
            .DisplayName(LOCTEXT("SceneKindLabel", "Scene Kind"));

        SceneKindHandle->SetOnPropertyValueChanged(
            FSimpleDelegate::CreateSP(this, &FSceneActorRefCustomization::OnSceneKindChanged));
    }

    // --- SceneFloor / SceneRegion ---
    // Порядок AddProperty совпадает с порядком в структуре.
    // Видимость каждого из них рулит EditCondition meta (UE 5.4 honours it in AddProperty).
    if (SceneFloorHandle.IsValid())
    {
        ChildBuilder.AddProperty(SceneFloorHandle.ToSharedRef())
            .DisplayName(LOCTEXT("SceneFloorLabel", "Scene Floor"));

        SceneFloorHandle->SetOnPropertyValueChanged(
            FSimpleDelegate::CreateSP(this, &FSceneActorRefCustomization::OnSceneChanged));
    }

    if (SceneRegionHandle.IsValid())
    {
        ChildBuilder.AddProperty(SceneRegionHandle.ToSharedRef())
            .DisplayName(LOCTEXT("SceneRegionLabel", "Scene Region"));

        SceneRegionHandle->SetOnPropertyValueChanged(
            FSimpleDelegate::CreateSP(this, &FSceneActorRefCustomization::OnSceneChanged));
    }

    // --- Actor dropdown ---
    ChildBuilder.AddCustomRow(LOCTEXT("ActorRow", "Actor"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(LOCTEXT("ActorName", "Actor"))
                .Font(IDetailLayoutBuilder::GetDetailFont())
        ]
        .ValueContent()
        .MinDesiredWidth(400.f)
        [
            SAssignNew(ActorComboBox, SComboBox<TSharedPtr<FGuid>>)
                .OptionsSource(&ActorOptions)
                .OnGenerateWidget(this, &FSceneActorRefCustomization::MakeActorComboItem)
                .OnSelectionChanged(this, &FSceneActorRefCustomization::OnActorSelected)
                .Content()
                [
                    SNew(STextBlock)
                        .Text(this, &FSceneActorRefCustomization::GetCurrentActorLabel)
                ]
        ];

    RebuildActorList();
}

// ─────────────────────────────────────────────────────────────────────────────

ESceneAssetKind FSceneActorRefCustomization::GetSceneKindFromHandle() const
{
    if (!SceneKindHandle.IsValid())
        return ESceneAssetKind::Floor;

    TArray<void*> RawData;
    SceneKindHandle->AccessRawData(RawData);
    if (RawData.Num() == 0 || !RawData[0])
        return ESceneAssetKind::Floor;

    return *static_cast<ESceneAssetKind*>(RawData[0]);
}

static UWorld* LoadWorldFromLevelPath(const FSoftObjectPath& LevelPath)
{
    if (!LevelPath.IsValid())
        return nullptr;

    const FString PackageName = LevelPath.GetLongPackageName();
    if (PackageName.IsEmpty())
        return nullptr;

    UPackage* Pkg = FindPackage(nullptr, *PackageName);
    if (!Pkg)
        Pkg = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Pkg)
        return nullptr;

    return UWorld::FindWorldInPackage(Pkg);
}

UWorld* FSceneActorRefCustomization::ResolveWorldFromSceneSelection() const
{
    const ESceneAssetKind Kind = GetSceneKindFromHandle();

    if (Kind == ESceneAssetKind::Floor)
    {
        if (!SceneFloorHandle.IsValid())
            return nullptr;

        TArray<void*> RawData;
        SceneFloorHandle->AccessRawData(RawData);
        if (RawData.Num() == 0 || !RawData[0])
            return nullptr;

        FSoftObjectPtr* SoftPtr = static_cast<FSoftObjectPtr*>(RawData[0]);
        TSoftObjectPtr<UFloorAsset> ScenePtr(SoftPtr->ToSoftObjectPath());
        if (ScenePtr.IsNull())
            return nullptr;

        UFloorAsset* Floor = ScenePtr.LoadSynchronous();
        if (!Floor || Floor->FloorLevel.IsNull())
            return nullptr;

        return LoadWorldFromLevelPath(Floor->FloorLevel.ToSoftObjectPath());
    }
    else // Region
    {
        if (!SceneRegionHandle.IsValid())
            return nullptr;

        TArray<void*> RawData;
        SceneRegionHandle->AccessRawData(RawData);
        if (RawData.Num() == 0 || !RawData[0])
            return nullptr;

        FSoftObjectPtr* SoftPtr = static_cast<FSoftObjectPtr*>(RawData[0]);
        TSoftObjectPtr<UWorldRegionAsset> ScenePtr(SoftPtr->ToSoftObjectPath());
        if (ScenePtr.IsNull())
            return nullptr;

        UWorldRegionAsset* Region = ScenePtr.LoadSynchronous();
        if (!Region || Region->RegionLevel.IsNull())
            return nullptr;

        return LoadWorldFromLevelPath(Region->RegionLevel.ToSoftObjectPath());
    }
}

FGuid FSceneActorRefCustomization::GetActorGuidFromHandle() const
{
    if (!ActorPersistentIdHandle.IsValid())
        return FGuid();

    TArray<void*> RawData;
    ActorPersistentIdHandle->AccessRawData(RawData);
    if (RawData.Num() == 0 || !RawData[0])
        return FGuid();

    return *static_cast<FGuid*>(RawData[0]);
}

void FSceneActorRefCustomization::SetActorGuidOnHandle(const FGuid& NewGuid) const
{
    if (!ActorPersistentIdHandle.IsValid())
        return;

    TArray<void*> RawData;
    ActorPersistentIdHandle->AccessRawData(RawData);
    if (RawData.Num() == 0 || !RawData[0])
        return;

    *static_cast<FGuid*>(RawData[0]) = NewGuid;
    ActorPersistentIdHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

// ─────────────────────────────────────────────────────────────────────────────

void FSceneActorRefCustomization::OnSceneKindChanged()
{
    // Сбрасываем выбор актёра — теперь сцена другая.
    SetActorGuidOnHandle(FGuid());

    if (ActorLabelHandle.IsValid())
        ActorLabelHandle->SetValue(FString());

    // Пересобираем список актёров для нового типа сцены.
    RebuildActorList();

    // Force refresh — чтобы сработал EditCondition у SceneFloor/SceneRegion.
    if (PropertyUtilities.IsValid())
        PropertyUtilities->ForceRefresh();
}

void FSceneActorRefCustomization::OnSceneChanged()
{
    SetActorGuidOnHandle(FGuid());

    if (ActorLabelHandle.IsValid())
        ActorLabelHandle->SetValue(FString());

    RebuildActorList();
}

void FSceneActorRefCustomization::RebuildActorList()
{
    ActorOptions.Empty();
    ActorLabels.Empty();

    UWorld* World = ResolveWorldFromSceneSelection();

    if (World && World->PersistentLevel)
    {
        for (AActor* Actor : World->PersistentLevel->Actors)
        {
            if (!IsValid(Actor)) continue;

            UFloorAssignmentComponent* FAC =
                Actor->FindComponentByClass<UFloorAssignmentComponent>();
            UInteractiveItemComponent* IIC =
                Actor->FindComponentByClass<UInteractiveItemComponent>();
            if (!FAC || !IIC || !FAC->ItemId.IsValid()) continue;

            const FGuid Id = FAC->ItemId;
            const FString Label = Actor->GetActorLabel();
            ActorOptions.Add(MakeShared<FGuid>(Id));
            ActorLabels.Add(Id, Label);
        }
    }

    // Восстанавливаем текущий выбор.
    const FGuid CurrentId = GetActorGuidFromHandle();
    TSharedPtr<FGuid> CurrentSelection;
    if (CurrentId.IsValid())
    {
        for (auto& Opt : ActorOptions)
        {
            if (*Opt == CurrentId) { CurrentSelection = Opt; break; }
        }
    }

    if (ActorComboBox.IsValid())
    {
        ActorComboBox->RefreshOptions();
        ActorComboBox->SetSelectedItem(CurrentSelection);
    }
}

void FSceneActorRefCustomization::OnActorSelected(
    TSharedPtr<FGuid> NewSelection, ESelectInfo::Type /*SelectInfo*/)
{
    if (!NewSelection.IsValid()) return;

    SetActorGuidOnHandle(*NewSelection);

    if (ActorLabelHandle.IsValid())
    {
        const FString* Found = ActorLabels.Find(*NewSelection);
        ActorLabelHandle->SetValue(Found ? *Found : FString());
    }
}

TSharedRef<SWidget> FSceneActorRefCustomization::MakeActorComboItem(TSharedPtr<FGuid> Item)
{
    FString Label;
    if (Item.IsValid())
    {
        const FString* Found = ActorLabels.Find(*Item);
        Label = Found ? *Found : Item->ToString();
    }
    return SNew(STextBlock).Text(FText::FromString(Label));
}

FText FSceneActorRefCustomization::GetCurrentActorLabel() const
{
    const FGuid Id = GetActorGuidFromHandle();
    if (!Id.IsValid())
        return LOCTEXT("None", "(select actor)");

    const FString* Found = ActorLabels.Find(Id);
    return FText::FromString(Found ? *Found : Id.ToString());
}

#undef LOCTEXT_NAMESPACE