#include "LocationAnchorActor.h"
#include "Components/BillboardComponent.h"
#include "WorldRegionAsset.h"
#include "FloorAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "UObject/ConstructorHelpers.h"
#include "LocationEditorUtils.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "Engine/World.h"
#include "Editor.h"
#include "Engine/Selection.h"
#endif

ALocationAnchorActor::ALocationAnchorActor()
{
    PrimaryActorTick.bCanEverTick = false;

    AnchorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("AnchorRoot"));
    RootComponent = AnchorRoot;

#if WITH_EDITOR
    // —оздаЄм обычный компонент только в редакторе Ч чтобы избежать ошибок при дочерних BP-компонентах
    EditorSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
    if (EditorSprite)
    {
        EditorSprite->SetupAttachment(AnchorRoot);
        EditorSprite->bIsScreenSizeScaled = true;
    }
#endif

    // √енераци€ GUID при создании
    AnchorID = FGuid::NewGuid();
}

// ∆изненные методы объ€влены всегда (прототипы в заголовке) Ч реализации вызывают editor-логику внутри #if
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

void ALocationAnchorActor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

#if WITH_EDITOR
    // при перемещении можно синхронизировать позицию в ассете (по желанию)
#endif
}

#if WITH_EDITOR

void ALocationAnchorActor::SyncToAsset()
{
    auto UpdateAnchorInArray = [this](TArray<FLocationAnchor>& Anchors) -> bool
    {
        for (FLocationAnchor& Anchor : Anchors)
        {
            if (Anchor.AnchorID == AnchorID)
            {
                Anchor.DisplayName = DisplayName;
                Anchor.WorldPosition = GetActorLocation();
                Anchor.WorldOrientation = GetActorRotation();
                Anchor.ReturnLink = DestinationLink;
                return true;
            }
        }

        FLocationAnchor NewAnchor;
        NewAnchor.AnchorID = AnchorID;
        NewAnchor.DisplayName = DisplayName;
        NewAnchor.WorldPosition = GetActorLocation();
        NewAnchor.WorldOrientation = GetActorRotation();
        NewAnchor.ReturnLink = DestinationLink;
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

    DestinationLink.TargetAnchorDisplayName = FText::FromString(TEXT("[якорь не найден]"));
}

void ALocationAnchorActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    FName PropName = PropertyChangedEvent.GetPropertyName();

    // ќбновление отображаемого имени при смене целевого €кор€/этажа/региона
    if (PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetAnchorID)
        || PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetFloor)
        || PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetRegion))
    {
        RefreshLinkDisplayName();
    }

    //  огда изменили TargetRegion или TargetFloor Ч собрать кандидатов и при единственном варианте автоназначить
    if (PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetRegion)
        || PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetFloor))
    {
        TArray<FGuid> CandidateIDs;
        TArray<FText> CandidateNames;

        if (!DestinationLink.TargetFloor.IsNull())
        {
            if (UFloorAsset* Floor = DestinationLink.TargetFloor.LoadSynchronous())
            {
                for (const FLocationAnchor& A : Floor->Anchors)
                {
                    CandidateIDs.Add(A.AnchorID);
                    CandidateNames.Add(A.DisplayName);
                }
            }
        }
        else if (!DestinationLink.TargetRegion.IsNull())
        {
            if (UWorldRegionAsset* Region = DestinationLink.TargetRegion.LoadSynchronous())
            {
                for (const FLocationAnchor& A : Region->Anchors)
                {
                    CandidateIDs.Add(A.AnchorID);
                    CandidateNames.Add(A.DisplayName);
                }
            }
        }

        UE_LOG(LogTemp, Verbose, TEXT("ALocationAnchorActor '%s' found %d candidate anchors for destination"), *GetName(), CandidateIDs.Num());

        if (CandidateIDs.Num() == 1)
        {
            DestinationLink.TargetAnchorID = CandidateIDs[0];
            RefreshLinkDisplayName();
            UE_LOG(LogTemp, Log, TEXT("ALocationAnchorActor '%s' auto-selected single candidate anchor '%s'"), *GetName(), *CandidateNames[0].ToString());

            // ѕопытка установить двустороннюю св€зь
            if (OwnerContextType == ELocationContextType::Street && !OwnerRegion.IsNull())
            {
                UWorldRegionAsset* SrcRegion = OwnerRegion.LoadSynchronous();
                if (SrcRegion && !DestinationLink.TargetFloor.IsNull())
                {
                    UFloorAsset* DestFloor = DestinationLink.TargetFloor.LoadSynchronous();
                    if (DestFloor)
                    {
                        ULocationEditorUtils::EstablishBidirectionalLink(SrcRegion, AnchorID, DestFloor, DestinationLink.TargetAnchorID);
                    }
                }
            }
            else if (OwnerContextType == ELocationContextType::Floor && !OwnerFloor.IsNull())
            {
                UFloorAsset* SrcFloor = OwnerFloor.LoadSynchronous();
                if (SrcFloor && !DestinationLink.TargetRegion.IsNull())
                {
                    UWorldRegionAsset* DestRegion = DestinationLink.TargetRegion.LoadSynchronous();
                    if (DestRegion)
                    {
                        ULocationEditorUtils::EstablishBidirectionalLink(DestRegion, DestinationLink.TargetAnchorID, SrcFloor, AnchorID);
                    }
                }
            }
        }
    }

    //  огда €вно изменили TargetAnchorID Ч сразу установить обратную ссылку в целевом ассете
    if (PropName == GET_MEMBER_NAME_CHECKED(FLocationAnchorLink, TargetAnchorID))
    {
        if (DestinationLink.TargetAnchorID.IsValid())
        {
            if (OwnerContextType == ELocationContextType::Street && !OwnerRegion.IsNull() && !DestinationLink.TargetFloor.IsNull())
            {
                UWorldRegionAsset* SrcRegion = OwnerRegion.LoadSynchronous();
                UFloorAsset* DestFloor = DestinationLink.TargetFloor.LoadSynchronous();
                if (SrcRegion && DestFloor)
                {
                    bool bOk = ULocationEditorUtils::EstablishBidirectionalLink(SrcRegion, AnchorID, DestFloor, DestinationLink.TargetAnchorID);
                    UE_LOG(LogTemp, Log, TEXT("ALocationAnchorActor '%s' EstablishBidirectionalLink(region->floor) result=%d"), *GetName(), (int)bOk);
                }
            }
            else if (OwnerContextType == ELocationContextType::Floor && !OwnerFloor.IsNull() && !DestinationLink.TargetRegion.IsNull())
            {
                UFloorAsset* SrcFloor = OwnerFloor.LoadSynchronous();
                UWorldRegionAsset* DestRegion = DestinationLink.TargetRegion.LoadSynchronous();
                if (SrcFloor && DestRegion)
                {
                    bool bOk = ULocationEditorUtils::EstablishBidirectionalLink(DestRegion, DestinationLink.TargetAnchorID, SrcFloor, AnchorID);
                    UE_LOG(LogTemp, Log, TEXT("ALocationAnchorActor '%s' EstablishBidirectionalLink(floor->region) result=%d"), *GetName(), (int)bOk);
                }
            }
        }
    }
}

bool ALocationAnchorActor::TryAutoAssignOwnerFromWorld()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("ALocationAnchorActor::TryAutoAssignOwnerFromWorld: no World()"));
        return false;
    }

    const FString CurrentWorldPackage = World->GetOutermost()->GetName();
    UE_LOG(LogTemp, Verbose, TEXT("TryAutoAssignOwnerFromWorld: world package = %s"), *CurrentWorldPackage);

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    // »щем FloorAsset
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UFloorAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;

        TArray<FAssetData> Assets;
        AssetRegistry.GetAssets(Filter, Assets);

        UE_LOG(LogTemp, Verbose, TEXT("TryAutoAssignOwnerFromWorld: found %d FloorAsset candidates"), Assets.Num());

        for (const FAssetData& AD : Assets)
        {
            UFloorAsset* FloorAsset = Cast<UFloorAsset>(AD.GetAsset());
            if (!FloorAsset)
            {
                UObject* Obj = AD.ToSoftObjectPath().TryLoad();
                FloorAsset = Cast<UFloorAsset>(Obj);
            }

            if (!FloorAsset)
            {
                UE_LOG(LogTemp, VeryVerbose, TEXT("TryAutoAssignOwnerFromWorld: failed to load FloorAsset %s"), *AD.GetObjectPathString());
                continue;
            }

            if (FloorAsset->FloorLevel.IsValid())
            {
                FSoftObjectPath Soft = FloorAsset->FloorLevel.ToSoftObjectPath();
                const FString FloorPkg = Soft.GetLongPackageName();
                UE_LOG(LogTemp, VeryVerbose, TEXT("Checking FloorAsset '%s' -> FloorLevel package '%s'"), *AD.AssetName.ToString(), *FloorPkg);
                if (FloorPkg == CurrentWorldPackage)
                {
                    OwnerContextType = ELocationContextType::Floor;
                    OwnerFloor = FloorAsset;
                    UE_LOG(LogTemp, Log, TEXT("Auto-assigned OwnerFloor = %s for actor %s"), *AD.AssetName.ToString(), *GetName());
                    return true;
                }
            }
        }
    }

    // »щем WorldRegionAsset
    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UWorldRegionAsset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;

        TArray<FAssetData> Assets;
        AssetRegistry.GetAssets(Filter, Assets);

        UE_LOG(LogTemp, Verbose, TEXT("TryAutoAssignOwnerFromWorld: found %d WorldRegionAsset candidates"), Assets.Num());

        for (const FAssetData& AD : Assets)
        {
            UWorldRegionAsset* RegionAsset = Cast<UWorldRegionAsset>(AD.GetAsset());
            if (!RegionAsset)
            {
                UObject* Obj = AD.ToSoftObjectPath().TryLoad();
                RegionAsset = Cast<UWorldRegionAsset>(Obj);
            }

            if (!RegionAsset)
            {
                UE_LOG(LogTemp, VeryVerbose, TEXT("TryAutoAssignOwnerFromWorld: failed to load RegionAsset %s"), *AD.GetObjectPathString());
                continue;
            }

            if (RegionAsset->RegionLevel.IsValid())
            {
                FSoftObjectPath Soft = RegionAsset->RegionLevel.ToSoftObjectPath();
                const FString RegionPkg = Soft.GetLongPackageName();
                UE_LOG(LogTemp, VeryVerbose, TEXT("Checking WorldRegionAsset '%s' -> RegionLevel package '%s'"), *AD.AssetName.ToString(), *RegionPkg);
                if (RegionPkg == CurrentWorldPackage)
                {
                    OwnerContextType = ELocationContextType::Street;
                    OwnerRegion = RegionAsset;
                    UE_LOG(LogTemp, Log, TEXT("Auto-assigned OwnerRegion = %s for actor %s"), *AD.AssetName.ToString(), *GetName());
                    return true;
                }
            }
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("TryAutoAssignOwnerFromWorld: no matching owner found for actor %s"), *GetName());
    return false;
}

#endif // WITH_EDITOR