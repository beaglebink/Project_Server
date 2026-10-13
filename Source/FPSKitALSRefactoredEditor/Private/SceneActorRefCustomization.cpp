#include "SceneActorRefCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"

#include "FloorAssignmentComponent.h"
#include "InteractiveItemComponent.h"
#include "SceneActorRef.h"
#include "WorldStateActorRef.h"
#include "FloorAsset.h"
#include "WorldRegionAsset.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "Styling/SlateColor.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/SoftObjectPath.h"

#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "SceneActorRefCustomization"

TSharedRef<IPropertyTypeCustomization> FSceneActorRefCustomization::MakeInstance()
{
    return MakeShareable(new FSceneActorRefCustomization);
}

// ─────────────────────────────────────────────────────────────────────────────
// Meta
// ─────────────────────────────────────────────────────────────────────────────

FSceneActorRefCustomization::EFilterMode FSceneActorRefCustomization::ReadFilterMode(
    const TSharedRef<IPropertyHandle>& StructPropertyHandle) const
{
    const FProperty* Prop = StructPropertyHandle->GetProperty();
    if (!Prop)
        return EFilterMode::InteractiveItem;

    const FString Meta = Prop->GetMetaData(TEXT("SceneActorFilter"));
    if (Meta.Equals(TEXT("FloorAssignmentOnly"), ESearchCase::IgnoreCase))
        return EFilterMode::FloorAssignmentOnly;

    return EFilterMode::InteractiveItem;
}

bool FSceneActorRefCustomization::ActorMatchesFilter(AActor* Actor, EFilterMode Mode) const
{
    if (!IsValid(Actor))
        return false;

    // Оба режима требуют FloorAssignmentComponent с валидным ItemId.
    UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>();
    if (!FAC || !FAC->ItemId.IsValid())
        return false;

    if (Mode == EFilterMode::InteractiveItem)
    {
        if (!Actor->FindComponentByClass<UInteractiveItemComponent>())
            return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// World resolution
// ─────────────────────────────────────────────────────────────────────────────

UWorld* FSceneActorRefCustomization::ResolveSceneWorld() const
{
    uint8 KindValue = 0;
    if (SceneKindHandle.IsValid())
    {
        SceneKindHandle->GetValue(KindValue);
    }

    const ESceneAssetKind Kind = static_cast<ESceneAssetKind>(KindValue);

    FSoftObjectPath LevelPath;

    if (Kind == ESceneAssetKind::Floor && SceneFloorHandle.IsValid())
    {
        FString FloorStr;
        SceneFloorHandle->GetValueAsFormattedString(FloorStr);
        if (!FloorStr.IsEmpty() && FloorStr != TEXT("None"))
        {
            UFloorAsset* Floor = Cast<UFloorAsset>(FSoftObjectPath(FloorStr).TryLoad());
            if (Floor && !Floor->FloorLevel.IsNull())
            {
                LevelPath = Floor->FloorLevel.ToSoftObjectPath();
            }
        }
    }
    else if (Kind == ESceneAssetKind::Region && SceneRegionHandle.IsValid())
    {
        FString RegionStr;
        SceneRegionHandle->GetValueAsFormattedString(RegionStr);
        if (!RegionStr.IsEmpty() && RegionStr != TEXT("None"))
        {
            UWorldRegionAsset* Region = Cast<UWorldRegionAsset>(FSoftObjectPath(RegionStr).TryLoad());
            if (Region && !Region->RegionLevel.IsNull())
            {
                LevelPath = Region->RegionLevel.ToSoftObjectPath();
            }
        }
    }

    if (!LevelPath.IsValid())
        return nullptr;

    const FString PackageName = LevelPath.GetLongPackageName();
    const FString WorldName = FString::Printf(TEXT("%s.%s"),
        *PackageName, *FPackageName::GetShortName(PackageName));

    if (UWorld* Existing = FindObject<UWorld>(nullptr, *WorldName))
        return Existing;

    UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
    if (!Package)
        return nullptr;

    return UWorld::FindWorldInPackage(Package);
}

// ─────────────────────────────────────────────────────────────────────────────
// Candidate collection
// ─────────────────────────────────────────────────────────────────────────────

TArray<AActor*> FSceneActorRefCustomization::CollectCandidateActors() const
{
    TArray<AActor*> Result;

    UWorld* SceneWorld = ResolveSceneWorld();
    if (!SceneWorld)
        return Result;

    for (TActorIterator<AActor> It(SceneWorld); It; ++It)
    {
        AActor* Actor = *It;
        if (ActorMatchesFilter(Actor, CurrentFilterMode))
        {
            Result.Add(Actor);
        }
    }

    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Label / selection
// ─────────────────────────────────────────────────────────────────────────────

FText FSceneActorRefCustomization::GetCurrentActorLabel() const
{
    FString LabelStr;
    if (ActorLabelHandle.IsValid())
    {
        ActorLabelHandle->GetValueAsFormattedString(LabelStr);
    }

    if (LabelStr.IsEmpty() || LabelStr == TEXT("None"))
    {
        return LOCTEXT("NoActorSelected", "<no actor selected>");
    }

    return FText::FromString(LabelStr);
}

FReply FSceneActorRefCustomization::OnActorSelected(TWeakObjectPtr<AActor> WeakActor)
{
    AActor* Actor = WeakActor.Get();
    if (!IsValid(Actor))
        return FReply::Handled();

    UFloorAssignmentComponent* FAC = Actor->FindComponentByClass<UFloorAssignmentComponent>();
    if (!FAC || !FAC->ItemId.IsValid())
        return FReply::Handled();

    if (PersistentIdHandle.IsValid())
    {
        PersistentIdHandle->SetValueFromFormattedString(FAC->ItemId.ToString());
    }
    if (ActorLabelHandle.IsValid())
    {
        ActorLabelHandle->SetValueFromFormattedString(Actor->GetActorLabel());
    }

    return FReply::Handled();
}

FReply FSceneActorRefCustomization::OnClearSelection()
{
    if (PersistentIdHandle.IsValid())
    {
        PersistentIdHandle->SetValueFromFormattedString(FGuid().ToString());
    }
    if (ActorLabelHandle.IsValid())
    {
        ActorLabelHandle->SetValueFromFormattedString(FString());
    }
    return FReply::Handled();
}

TSharedRef<SWidget> FSceneActorRefCustomization::OnGenerateActorMenu()
{
    TArray<AActor*> Candidates = CollectCandidateActors();

    TSharedRef<SVerticalBox> List = SNew(SVerticalBox);

    // Первая строка — сброс выбора.
    List->AddSlot()
        .AutoHeight()
        .Padding(2)
        [
            SNew(SButton)
                .HAlign(HAlign_Left)
                .OnClicked(FOnClicked::CreateLambda([this]() -> FReply
                    {
                        return OnClearSelection();
                    }))
                [
                    SNew(STextBlock).Text(LOCTEXT("ClearSelection", "<clear>"))
                ]
        ];

    if (Candidates.Num() == 0)
    {
        List->AddSlot()
            .AutoHeight()
            .Padding(4)
            [
                SNew(STextBlock)
                    .Text(LOCTEXT("NoActorsFound", "No matching actors on the selected level"))
            ];
    }
    else
    {
        for (AActor* Actor : Candidates)
        {
            if (!IsValid(Actor))
                continue;

            const FString Label = Actor->GetActorLabel();
            TWeakObjectPtr<AActor> WeakActor(Actor);

            List->AddSlot()
                .AutoHeight()
                .Padding(2)
                [
                    SNew(SButton)
                        .HAlign(HAlign_Left)
                        .OnClicked(FOnClicked::CreateLambda([this, WeakActor]() -> FReply
                            {
                                return OnActorSelected(WeakActor);
                            }))
                        [
                            SNew(STextBlock).Text(FText::FromString(Label))
                        ]
                ];
        }
    }

    return SNew(SBox)
        .MaxDesiredHeight(400.f)
        .MinDesiredWidth(300.f)
        [
            SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    List
                ]
        ];
}

// ─────────────────────────────────────────────────────────────────────────────
// IPropertyTypeCustomization
// ─────────────────────────────────────────────────────────────────────────────

void FSceneActorRefCustomization::CustomizeHeader(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    FDetailWidgetRow& HeaderRow,
    IPropertyTypeCustomizationUtils& Utils)
{
    StructHandle = StructPropertyHandle;
    CurrentFilterMode = ReadFilterMode(StructPropertyHandle);

    // Оба типа (FSceneActorRef и FWorldStateActorRef) имеют одинаковые поля
    // по FName, поэтому lookup через GET_MEMBER_NAME_CHECKED подходит обоим.
    SceneKindHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, SceneKind));
    SceneFloorHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, SceneFloor));
    SceneRegionHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, SceneRegion));
    PersistentIdHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, ActorPersistentId));
    ActorLabelHandle = StructPropertyHandle->GetChildHandle(
        GET_MEMBER_NAME_CHECKED(FSceneActorRef, ActorLabel));

    // Header — только имя и subdued-превью текущего выбора.
    // Сам пикер живёт ниже, в children.
    HeaderRow
        .NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        .MinDesiredWidth(200.f)
        [
            SNew(STextBlock)
                .Text_Lambda([this]() -> FText
                    {
                        return GetCurrentActorLabel();
                    })
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];
}

void FSceneActorRefCustomization::CustomizeChildren(
    TSharedRef<IPropertyHandle> StructPropertyHandle,
    IDetailChildrenBuilder& ChildBuilder,
    IPropertyTypeCustomizationUtils& Utils)
{
    // 1. Scene Kind
    if (SceneKindHandle.IsValid())
    {
        ChildBuilder.AddProperty(SceneKindHandle.ToSharedRef());
    }

    // 2. Scene Floor / Scene Region.
    //    Показываем оба — EditConditionHides на UPROPERTY скроет
    //    неподходящий вариант в зависимости от SceneKind.
    if (SceneFloorHandle.IsValid())
    {
        ChildBuilder.AddProperty(SceneFloorHandle.ToSharedRef());
    }
    if (SceneRegionHandle.IsValid())
    {
        ChildBuilder.AddProperty(SceneRegionHandle.ToSharedRef());
    }

    // 3. Actor picker — кастомный ряд.
    ChildBuilder.AddCustomRow(LOCTEXT("ActorPickSearch", "Actor"))
        .NameContent()
        [
            SNew(STextBlock)
                .Text(LOCTEXT("ActorPickLabel", "Actor"))
        ]
        .ValueContent()
        .MinDesiredWidth(300.f)
        [
            SNew(SComboButton)
                .OnGetMenuContent(FOnGetContent::CreateSP(this, &FSceneActorRefCustomization::OnGenerateActorMenu))
                .ButtonContent()
                [
                    SNew(STextBlock)
                        .Text_Lambda([this]() -> FText
                            {
                                return GetCurrentActorLabel();
                            })
                ]
        ];

    // 4. ActorPersistentId и ActorLabel сознательно не добавляем в children —
    //    они служебные и заполняются автоматически через пикер.
}

#undef LOCTEXT_NAMESPACE