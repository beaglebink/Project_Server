#include "LocationAnchorActor.h"
#include "Components/BillboardComponent.h"
#include "WorldRegionAsset.h"
#include "FloorAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "LocationEditorUtils.h"
#include "../InteriorInstanceSystem/InteriorTransitionPayload.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#endif

ALocationAnchorActor::ALocationAnchorActor()
{
    PrimaryActorTick.bCanEverTick = false;

    AnchorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorRoot"));
    RootComponent = AnchorRoot;

#if WITH_EDITOR
    EditorSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
    if (EditorSprite)
    {
        EditorSprite->SetupAttachment(AnchorRoot);
        EditorSprite->bIsScreenSizeScaled = true;
    }
#endif

    AnchorID = FGuid::NewGuid();
}

void ALocationAnchorActor::PostActorCreated()
{
    Super::PostActorCreated();
#if WITH_EDITOR
    TryAutoAssignOwnerFromWorld();
#endif
}

void ALocationAnchorActor::PostLoad()
{
    Super::PostLoad();
#if WITH_EDITOR
    if (GIsEditor)
    {
        TryAutoAssignOwnerFromWorld();
    }
#endif
}

void ALocationAnchorActor::Destroyed()
{
    Super::Destroyed();

#if WITH_EDITOR
    if (GIsEditor && AnchorID.IsValid())
    {
        const int32 Removed = ULocationEditorUtils::RemoveTransitionPointFromAllAssets(AnchorID);
        if (Removed > 0)
        {
            const FString AnchorDisplayLabel = DisplayName.IsEmpty()
                ? GetName()
                : DisplayName.ToString();

            const FText NotifyText = FText::Format(
                NSLOCTEXT("LocationAnchor", "RemovedOnDelete",
                    "Якорь \"{0}\" был зарегистрирован — {1} запис(ей) TransitionPoint удалено из ассетов."),
                FText::FromString(AnchorDisplayLabel),
                FText::AsNumber(Removed));

            FNotificationInfo Info(NotifyText);
            Info.bFireAndForget       = true;
            Info.ExpireDuration       = 6.0f;
            Info.bUseSuccessFailIcons = true;
            Info.Image = FCoreStyle::Get().GetBrush(TEXT("Icons.WarningWithColor"));

            TSharedPtr<SNotificationItem> Notification =
                FSlateNotificationManager::Get().AddNotification(Info);
            if (Notification.IsValid())
            {
                Notification->SetCompletionState(SNotificationItem::CS_Fail);
            }

            UE_LOG(LogTemp, Warning,
                TEXT("ALocationAnchorActor::Destroyed — актор '%s' [%s] удалён со сцены, "
                     "убрано %d TransitionPoint-записей из ассетов."),
                *AnchorDisplayLabel, *AnchorID.ToString(), Removed);
        }
    }
#endif
}

// ── Runtime реализации ────────────────────────────────────────────────────

UObject* ALocationAnchorActor::GetOwnerRegistrationAsset() const
{
    // Приоритет: Floor > InteriorSet > Street > Region
    // OwnerContextType намеренно игнорируется — используем только заполненные поля.
    if (!OwnerFloor.IsNull())
        return OwnerFloor.LoadSynchronous();

    if (!OwnerInteriorSet.IsNull())
        return OwnerInteriorSet.LoadSynchronous();

    if (!OwnerStreet.IsNull())
        return OwnerStreet.LoadSynchronous();

    if (!OwnerRegion.IsNull())
        return OwnerRegion.LoadSynchronous();

    return nullptr;
}

FInteriorTransitionDescriptor ALocationAnchorActor::MakeTransitionDescriptor() const
{
    FInteriorTransitionDescriptor Desc;
    if (!OwnerFloor.IsNull())
        Desc.SourceFloor = OwnerFloor;

    Desc.TargetRegion      = DestinationLink.TargetRegion;
    Desc.TargetStreet      = DestinationLink.TargetStreet;
    Desc.TargetInteriorSet = DestinationLink.TargetInteriorSet;
    Desc.TargetFloor       = DestinationLink.TargetFloor;
    Desc.TargetAnchorID    = DestinationLink.TargetAnchorID;
    Desc.TransitionPointId.Invalidate();
    Desc.AnchorIndex = -1;
    Desc.AnchorName  = NAME_None;
    return Desc;
}

UInteriorTransitionPayload* ALocationAnchorActor::CreateTransitionPayload() const
{
    UInteriorTransitionPayload* P = NewObject<UInteriorTransitionPayload>(GetTransientPackage(), NAME_None);
    if (P)
        P->Setup(DestinationLink);
    return P;
}

// ── Editor-only реализации ────────────────────────────────────────────────
#if WITH_EDITOR

void ALocationAnchorActor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);
}

void ALocationAnchorActor::SyncToAsset()
{
    auto UpdateAnchorInArray = [this](TArray<FLocationAnchor>& Anchors) -> bool
    {
        for (FLocationAnchor& Anchor : Anchors)
        {
            if (Anchor.AnchorID == AnchorID)
            {
                Anchor.DisplayName      = DisplayName;
                Anchor.WorldPosition    = GetActorLocation();
                Anchor.WorldOrientation = GetActorRotation();
                Anchor.ReturnLink       = DestinationLink;
                return true;
            }
        }
        FLocationAnchor NewAnchor;
        NewAnchor.AnchorID          = AnchorID;
        NewAnchor.DisplayName       = DisplayName;
        NewAnchor.WorldPosition     = GetActorLocation();
        NewAnchor.WorldOrientation  = GetActorRotation();
        NewAnchor.ReturnLink        = DestinationLink;
        Anchors.Add(NewAnchor);
        return true;
    };

    if (OwnerContextType == ELocationContextType::Street && !OwnerRegion.IsNull())
    {
        if (UWorldRegionAsset* Region = OwnerRegion.LoadSynchronous())
        {
            UpdateAnchorInArray(Region->Anchors);
            Region->MarkPackageDirty();
        }
    }
    else if (OwnerContextType == ELocationContextType::Floor && !OwnerFloor.IsNull())
    {
        if (UFloorAsset* Floor = OwnerFloor.LoadSynchronous())
        {
            UpdateAnchorInArray(Floor->Anchors);
            Floor->MarkPackageDirty();
        }
    }
}

void ALocationAnchorActor::RefreshLinkDisplayName()
{
    if (!DestinationLink.TargetAnchorID.IsValid())
    {
        DestinationLink.TargetAnchorDisplayName = FText::GetEmpty();
        return;
    }

    if (!DestinationLink.TargetFloor.IsNull())
    {
        if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
        {
            for (const FLocationAnchor& A : Floor->Anchors)
            {
                if (A.AnchorID == DestinationLink.TargetAnchorID)
                {
                    DestinationLink.TargetAnchorDisplayName = A.DisplayName;
                    return;
                }
            }
        }
    }

    if (!DestinationLink.TargetRegion.IsNull())
    {
        if (UWorldRegionAsset* Region = DestinationLink.TargetRegion.LoadSynchronous())
        {
            for (const FLocationAnchor& A : Region->Anchors)
            {
                if (A.AnchorID == DestinationLink.TargetAnchorID)
                {
                    DestinationLink.TargetAnchorDisplayName = A.DisplayName;
                    return;
                }
            }
        }
    }

    DestinationLink.TargetAnchorDisplayName = FText::FromString(TEXT("[not in asset]"));
}

void ALocationAnchorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropName = PropertyChangedEvent.GetPropertyName();

    if (PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetAnchorID)
        || PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetFloor)
        || PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetRegion))
    {
        RefreshLinkDisplayName();
    }

    if (PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetRegion)
        || PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetFloor))
    {
        TArray<FGuid> CandidateIDs;
        TArray<FText> CandidateNames;

        if (!DestinationLink.TargetFloor.IsNull())
        {
            if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
                for (const FLocationAnchor& A : Floor->Anchors)
                { CandidateIDs.Add(A.AnchorID); CandidateNames.Add(A.DisplayName); }
        }
        else if (!DestinationLink.TargetRegion.IsNull())
        {
            if (UWorldRegionAsset* Region = DestinationLink.TargetRegion.LoadSynchronous())
                for (const FLocationAnchor& A : Region->Anchors)
                { CandidateIDs.Add(A.AnchorID); CandidateNames.Add(A.DisplayName); }
        }

        if (CandidateIDs.Num() == 1)
        {
            DestinationLink.TargetAnchorID = CandidateIDs[0];
            RefreshLinkDisplayName();

            if (OwnerContextType == ELocationContextType::Street && !OwnerRegion.IsNull())
            {
                if (UWorldRegionAsset* SrcRegion = OwnerRegion.LoadSynchronous())
                    if (!DestinationLink.TargetFloor.IsNull())
                        if (UFloorAsset* DestFloor = DestinationLink.TargetFloor.LoadSynchronous())
                            ULocationEditorUtils::EstablishBidirectionalLink(SrcRegion, AnchorID, DestFloor, DestinationLink.TargetAnchorID);
            }
            else if (OwnerContextType == ELocationContextType::Floor && !OwnerFloor.IsNull())
            {
                if (UFloorAsset* SrcFloor = OwnerFloor.LoadSynchronous())
                    if (!DestinationLink.TargetRegion.IsNull())
                        if (UWorldRegionAsset* DestRegion = DestinationLink.TargetRegion.LoadSynchronous())
                            ULocationEditorUtils::EstablishBidirectionalLink(DestRegion, DestinationLink.TargetAnchorID, SrcFloor, AnchorID);
            }
        }
    }

    if (PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetAnchorID))
    {
        if (DestinationLink.TargetAnchorID.IsValid())
        {
            if (OwnerContextType == ELocationContextType::Street && !OwnerRegion.IsNull() && !DestinationLink.TargetFloor.IsNull())
            {
                if (UWorldRegionAsset* SrcRegion = OwnerRegion.LoadSynchronous())
                    if (UFloorAsset* DestFloor = DestinationLink.TargetFloor.LoadSynchronous())
                        ULocationEditorUtils::EstablishBidirectionalLink(SrcRegion, AnchorID, DestFloor, DestinationLink.TargetAnchorID);
            }
            else if (OwnerContextType == ELocationContextType::Floor && !OwnerFloor.IsNull() && !DestinationLink.TargetRegion.IsNull())
            {
                if (UFloorAsset* SrcFloor = OwnerFloor.LoadSynchronous())
                    if (UWorldRegionAsset* DestRegion = DestinationLink.TargetRegion.LoadSynchronous())
                        ULocationEditorUtils::EstablishBidirectionalLink(DestRegion, DestinationLink.TargetAnchorID, SrcFloor, AnchorID);
            }
        }
    }
}

bool ALocationAnchorActor::TryAutoAssignOwnerFromWorld()
{
    UWorld* World = GetWorld();
    if (!World) return false;

    const FString CurrentWorldPackage = World->GetOutermost()->GetName();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UFloorAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AssetRegistry.GetAssets(Filter, Assets);

        for (const FAssetData& AD : Assets)
        {
            UFloorAsset* FloorAsset = Cast<UFloorAsset>(AD.GetAsset());
            if (!FloorAsset) FloorAsset = Cast<UFloorAsset>(AD.ToSoftObjectPath().TryLoad());
            if (!FloorAsset) continue;

            if (FloorAsset->FloorLevel.IsValid())
            {
                const FString FloorPkg = FloorAsset->FloorLevel.ToSoftObjectPath().GetLongPackageName();
                if (FloorPkg == CurrentWorldPackage)
                {
                    OwnerContextType = ELocationContextType::Floor;
                    OwnerFloor = FloorAsset;
                    return true;
                }
            }
        }
    }

    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UWorldRegionAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        AssetRegistry.GetAssets(Filter, Assets);

        for (const FAssetData& AD : Assets)
        {
            UWorldRegionAsset* RegionAsset = Cast<UWorldRegionAsset>(AD.GetAsset());
            if (!RegionAsset) RegionAsset = Cast<UWorldRegionAsset>(AD.ToSoftObjectPath().TryLoad());
            if (!RegionAsset) continue;

            if (RegionAsset->RegionLevel.IsValid())
            {
                const FString RegionPkg = RegionAsset->RegionLevel.ToSoftObjectPath().GetLongPackageName();
                if (RegionPkg == CurrentWorldPackage)
                {
                    OwnerContextType = ELocationContextType::Street;
                    OwnerRegion = RegionAsset;
                    return true;
                }
            }
        }
    }

    return false;
}

#endif // WITH_EDITOR