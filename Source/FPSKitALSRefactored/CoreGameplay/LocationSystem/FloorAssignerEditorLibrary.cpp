// This file intentionally left minimal — implementation moved to the Editor module:
// Source/FPSKitALSRefactoredEditor/Private/FloorAssignerEditorLibrary.cpp
// Keeping empty stub here prevents duplicate symbol issues during editor build.

#include "FloorAssignerEditorLibrary.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "WorldMapAsset.h"
#include "WorldRegionAsset.h"
#include "StreetAsset.h"
#include "InteriorSetAsset.h"
#include "FloorAsset.h"
#include "Engine/Selection.h"
#include "Editor.h"
#include "GameFramework/Actor.h"
#include "../InteriorInstanceSystem/FloorAssignmentComponent.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "EngineUtils.h"
#include "Editor/EditorEngine.h"
#include "../EventBusSystem/EventBusSubsystem.h"
#include "../InteriorInstanceSystem/FloorPlacementPayload.h"
#include "Components/ChildActorComponent.h"
#include "LocationEditorUtils.h"
#include "LocationAnchorActor.h"
#include "InteractiveItemComponent.h"
#include "ScopedTransaction.h"

// -----------------------------------------------------------------------------
// Helpers (identical to previous working implementation)
// -----------------------------------------------------------------------------
static TArray<FAssetData> GetAssetDataByClassName(const FString& ClassName)
{
    TArray<FAssetData> Result;
    FAssetRegistryModule& ARM = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
    ARM.Get().GetAssetsByClass(FTopLevelAssetPath(TEXT("/Script/FPSKitALSRefactored"), *ClassName), Result, true);
    return Result;
}

// Forward declaration: implemented further below
static int32 UpdateTransitionPointsForAsset(UObject* Asset, const TArray<FSoftObjectPath>& LevelPaths);
// Forward declarations for anchor lookup helpers (defined further below)
static ALocationAnchorActor* FindAnchorOnLevel(const FGuid& AnchorID, const FSoftObjectPath& LevelSoftPath);
static ALocationAnchorActor* FindAnchorInLevelPaths(const FGuid& AnchorID, const TArray<FSoftObjectPath>& Paths);
// Forward declarations for scene anchor collectors
static void GetAnchorsOnLevel(const FSoftObjectPath& LevelSoftPath, TArray<ALocationAnchorActor*>& OutAnchors);
static void GetAnchorsInLevelPaths(const TArray<FSoftObjectPath>& Paths, TArray<ALocationAnchorActor*>& OutAnchors);

int32 FFloorAssignerEditorLibrary::ValidateRegionTransitions(const FGuid& RegionGuid)
{
    if (!RegionGuid.IsValid()) return 0;

    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldRegionAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UWorldRegionAsset* Region = Cast<UWorldRegionAsset>(AD.GetAsset());
        if (!Region) Region = Cast<UWorldRegionAsset>(Path.TryLoad());
        if (!Region) continue;
        if (Region->WorldRegionID != RegionGuid) continue;

        TArray<FSoftObjectPath> LevelPaths;
        if (!Region->RegionLevel.IsNull()) LevelPaths.Add(Region->RegionLevel.ToSoftObjectPath());

        // First: scan scene level for anchors that exist on the level but are not yet registered in the asset
        TArray<ALocationAnchorActor*> SceneAnchors;
        GetAnchorsInLevelPaths(LevelPaths, SceneAnchors);
        TArray<FString> UnregisteredShort;
        for (ALocationAnchorActor* Anchor : SceneAnchors)
        {
            if (!IsValid(Anchor) || !Anchor->AnchorID.IsValid()) continue;
            bool bFound = false;
            for (const FLocationTransitionPoint& TP : Region->TransitionPoints)
            {
                if (TP.TransitionPointID == Anchor->AnchorID) { bFound = true; break; }
            }
            if (!bFound && (Anchor->RequiresDestination))
            {
                UnregisteredShort.Add(FString::Printf(TEXT("%s (%s) — Region: %s"), *Anchor->GetName(), *Anchor->DisplayName.ToString(), *Region->DisplayName.ToString()));
                UE_LOG(LogTemp, Warning, TEXT("ValidateRegionTransitions: Anchor on level not registered in Region asset: Actor=%s, Asset=%s, Pos=%s"), *Anchor->GetName(), *Region->GetName(), *Anchor->GetActorLocation().ToString());
            }
        }
        if (UnregisteredShort.Num() > 0)
        {
            FString Msg;
            for (const FString& S : UnregisteredShort) { Msg += S + TEXT("\n"); }
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Found %d anchors on scene not registered in Region asset:\n%s"), UnregisteredShort.Num(), *Msg)));
        }

        int32 Removed = ULocationEditorUtils::ValidateAndCleanTransitionPoints(Region);
        int32 Updated = UpdateTransitionPointsForAsset(Region, LevelPaths);

        // Additional check: validate that transition points that point to other anchors have reciprocal links
        TArray<FString> ShortMessages;
        for (const FLocationTransitionPoint& TP : Region->TransitionPoints)
        {
            if (!TP.DestinationLink.TargetAnchorID.IsValid()) continue;

            // Try to locate source actor
            ALocationAnchorActor* SourceActor = nullptr;
            if (!TP.SourceAnchor.IsNull()) SourceActor = Cast<ALocationAnchorActor>(TP.SourceAnchor.Get());
            if (!SourceActor) SourceActor = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths);

            // Resolve destination level path
            FSoftObjectPath DestLevelPath;
            if (!TP.DestinationLink.TargetFloor.IsNull())
            {
                if (UFloorAsset* F = TP.DestinationLink.TargetFloor.LoadSynchronous())
                    if (!F->FloorLevel.IsNull())
                        DestLevelPath = F->FloorLevel.ToSoftObjectPath();
            }
            if (!DestLevelPath.IsValid() && !TP.DestinationLink.TargetRegion.IsNull())
            {
                if (UWorldRegionAsset* R = TP.DestinationLink.TargetRegion.LoadSynchronous())
                    if (!R->RegionLevel.IsNull())
                        DestLevelPath = R->RegionLevel.ToSoftObjectPath();
            }

            ALocationAnchorActor* DestActor = nullptr;
            if (DestLevelPath.IsValid()) DestActor = FindAnchorOnLevel(TP.DestinationLink.TargetAnchorID, DestLevelPath);

            bool bReciprocal = false;
            if (DestActor)
            {
                // Direct destination link back to this TP
                if (DestActor->DestinationLink.TargetAnchorID == TP.TransitionPointID)
                {
                    bReciprocal = true;
                }
                else
                {
                    // Check anchors stored in the owner registration asset (if available)
                    if (UObject* DAReg = DestActor->GetOwnerRegistrationAsset())
                    {
                        // WorldRegion
                        if (UWorldRegionAsset* Reg = Cast<UWorldRegionAsset>(DAReg))
                        {
                            for (const FLocationAnchor& AnchorEntry : Reg->Anchors)
                            {
                                if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID)
                                {
                                    bReciprocal = true;
                                    break;
                                }
                            }
                        }
                        // Street
                        else if (UStreetAsset* StreetOwner = Cast<UStreetAsset>(DAReg))
                        {
                            for (const FLocationAnchor& AnchorEntry : StreetOwner->Anchors)
                            {
                                if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID)
                                {
                                    bReciprocal = true;
                                    break;
                                }
                            }
                        }
                        // InteriorSet: no direct Anchors array on InteriorSet; check floors
                        else if (UInteriorSetAsset* InteriorOwner = Cast<UInteriorSetAsset>(DAReg))
                        {
                            for (const TSoftObjectPtr<UFloorAsset>& FloorRef : InteriorOwner->Floors)
                            {
                                if (!FloorRef.IsValid()) continue;
                                UFloorAsset* FloorObj = FloorRef.LoadSynchronous();
                                if (!FloorObj) continue;
                                for (const FLocationAnchor& AnchorEntry : FloorObj->Anchors)
                                {
                                    if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID)
                                    {
                                        bReciprocal = true;
                                        break;
                                    }
                                }
                                if (bReciprocal) break;
                            }
                        }
                        // Floor
                        else if (UFloorAsset* FloorOwner = Cast<UFloorAsset>(DAReg))
                        {
                            for (const FLocationAnchor& AnchorEntry : FloorOwner->Anchors)
                            {
                                if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID)
                                {
                                    bReciprocal = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            if (!bReciprocal)
            {
                FString ObjName = SourceActor ? SourceActor->GetName() : TEXT("<no actor>");
                FString Disp = SourceActor ? SourceActor->DisplayName.ToString() : TP.DisplayName.ToString();
                FString Loc = FString::Printf(TEXT("Region: %s"), *Region->DisplayName.ToString());
                ShortMessages.Add(FString::Printf(TEXT("%s (%s) — %s"), *ObjName, *Disp, *Loc));

                // Log detailed info
                FVector Pos = SourceActor ? SourceActor->GetActorLocation() : TP.SourceWorldPosition;
                UE_LOG(LogTemp, Warning, TEXT("ValidateRegionTransitions: Non-reciprocal anchor '%s' (%s) in region asset '%s' at pos %s -> dest anchor id %s"),
                    *ObjName, *Disp, *Region->GetName(), *Pos.ToString(), *TP.DestinationLink.TargetAnchorID.ToString());
            }
        }

        if (ShortMessages.Num() > 0)
        {
            FString Msg;
            for (const FString& S : ShortMessages) { Msg += S + TEXT("\n"); }
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg));
        }

        UE_LOG(LogTemp, Log, TEXT("ValidateRegionTransitions: Region '%s' removed %d, updated %d transition points"), *Region->GetName(), Removed, Updated);
        return Removed + Updated;
    }

    UE_LOG(LogTemp, Warning, TEXT("ValidateRegionTransitions: Region with GUID %s not found"), *RegionGuid.ToString());
    return 0;
}

static FText ResolveFloorDisplayName(const FGuid& FloorGuid)
{
    if (!FloorGuid.IsValid())
    {
        return FText::FromString(TEXT("Invalid GUID"));
    }

    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
        if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
        if (!IS) continue;
        for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
        {
            UFloorAsset* Floor = Ref.LoadSynchronous();
            if (!Floor) continue;
            if (Floor->FloorID != FloorGuid) continue;

            if (!Floor->DisplayName.IsEmpty())
            {
                return Floor->DisplayName;
            }
            // fallback to asset name
            return FText::FromName(Floor->GetFName());
        }
    }
    // Not found: fallback to GUID string
    return FText::FromString(FloorGuid.ToString());
}

static ALocationAnchorActor* FindAnchorOnLevel(const FGuid& AnchorID, const FSoftObjectPath& LevelSoftPath)
{
    if (!AnchorID.IsValid() || !LevelSoftPath.IsValid()) return nullptr;
    const FString PackageName = LevelSoftPath.GetLongPackageName();
    if (PackageName.IsEmpty()) return nullptr;
    UPackage* Pkg = FindPackage(nullptr, *PackageName);
    if (!Pkg) Pkg = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Pkg) return nullptr;
    UWorld* World = UWorld::FindWorldInPackage(Pkg);
    if (!World || !World->PersistentLevel) return nullptr;
    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        if (ALocationAnchorActor* A = Cast<ALocationAnchorActor>(Actor))
        {
            if (A->AnchorID == AnchorID) return A;
        }
    }
    return nullptr;
}

static ALocationAnchorActor* FindAnchorInLevelPaths(const FGuid& AnchorID, const TArray<FSoftObjectPath>& Paths)
{
    for (const FSoftObjectPath& P : Paths)
    {
        if (!P.IsValid()) continue;
        if (ALocationAnchorActor* A = FindAnchorOnLevel(AnchorID, P))
            return A;
    }
    return nullptr;
}

// Collect all anchor actors on a single level
static void GetAnchorsOnLevel(const FSoftObjectPath& LevelSoftPath, TArray<ALocationAnchorActor*>& OutAnchors)
{
    if (!LevelSoftPath.IsValid()) return;
    const FString PackageName = LevelSoftPath.GetLongPackageName();
    if (PackageName.IsEmpty()) return;
    UPackage* Pkg = FindPackage(nullptr, *PackageName);
    if (!Pkg) Pkg = LoadPackage(nullptr, *PackageName, LOAD_NoWarn | LOAD_Quiet);
    if (!Pkg) return;
    UWorld* World = UWorld::FindWorldInPackage(Pkg);
    if (!World || !World->PersistentLevel) return;
    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        if (!IsValid(Actor)) continue;
        if (ALocationAnchorActor* Anchor = Cast<ALocationAnchorActor>(Actor))
        {
            OutAnchors.Add(Anchor);
        }
    }
}

static void GetAnchorsInLevelPaths(const TArray<FSoftObjectPath>& Paths, TArray<ALocationAnchorActor*>& OutAnchors)
{
    for (const FSoftObjectPath& P : Paths)
    {
        GetAnchorsOnLevel(P, OutAnchors);
    }
}

static void SetAnchorInteractiveEnabled(ALocationAnchorActor* Anchor, bool bEnable)
{
    if (!IsValid(Anchor)) return;
    if (UInteractiveItemComponent* Comp = Anchor->FindComponentByClass<UInteractiveItemComponent>())
    {
        Comp->SetIsActive(bEnable);
    }
}

static void ResolveDestinationActorInTP(FLocationTransitionPoint& TP)
{
    if (!TP.DestinationLink.TargetAnchorID.IsValid()) return;

    FSoftObjectPath DestLevelPath;
    if (!TP.DestinationLink.TargetFloor.IsNull())
    {
        if (UFloorAsset* F = TP.DestinationLink.TargetFloor.LoadSynchronous())
            if (!F->FloorLevel.IsNull())
                DestLevelPath = F->FloorLevel.ToSoftObjectPath();
    }
    if (!DestLevelPath.IsValid() && !TP.DestinationLink.TargetRegion.IsNull())
    {
        if (UWorldRegionAsset* R = TP.DestinationLink.TargetRegion.LoadSynchronous())
            if (!R->RegionLevel.IsNull())
                DestLevelPath = R->RegionLevel.ToSoftObjectPath();
    }

    if (!DestLevelPath.IsValid()) return;

    if (ALocationAnchorActor* DestA = FindAnchorOnLevel(TP.DestinationLink.TargetAnchorID, DestLevelPath))
    {
        TP.DestinationAnchor = TSoftObjectPtr<UObject>(DestA);
        TP.DestinationLink.TargetAnchorActor = TSoftObjectPtr<UObject>(DestA);
    }
}

static int32 UpdateTransitionPointsForAsset(UObject* Asset, const TArray<FSoftObjectPath>& LevelPaths)
{
    if (!Asset) return 0;
    int32 Updated = 0;

    auto UpdateFromPoints = [&](TArray<FLocationTransitionPoint>& Points, UObject* OwnerAsset)
    {
        bool bOwnerModified = false;
        for (FLocationTransitionPoint& TP : Points)
        {
            bool bChanged = false;

            if (!TP.SourceAnchor.IsNull())
            {
                UObject* Obj = TP.SourceAnchor.Get();
                if (Obj)
                {
                    if (ALocationAnchorActor* SA = Cast<ALocationAnchorActor>(Obj))
                    {
                        if (IsValid(SA) && SA->AnchorID == TP.TransitionPointID)
                        {
                            FVector NewPos = SA->GetActorLocation();
                            FRotator NewRot = SA->GetActorRotation();
                            if (!TP.SourceWorldPosition.Equals(NewPos) || !TP.SourceWorldOrientation.Equals(NewRot))
                            {
                                TP.SourceWorldPosition = NewPos;
                                TP.SourceWorldOrientation = NewRot;
                                bChanged = true;
                            }
                        }
                        else
                        {
                            if (ALocationAnchorActor* Found = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths))
                            {
                                TP.SourceAnchor = TSoftObjectPtr<UObject>(Found);
                                TP.SourceWorldPosition = Found->GetActorLocation();
                                TP.SourceWorldOrientation = Found->GetActorRotation();
                                bChanged = true;
                            }
                        }
                    }
                    else
                    {
                        if (ALocationAnchorActor* Found = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths))
                        {
                            TP.SourceAnchor = TSoftObjectPtr<UObject>(Found);
                            TP.SourceWorldPosition = Found->GetActorLocation();
                            TP.SourceWorldOrientation = Found->GetActorRotation();
                            bChanged = true;
                        }
                    }
                }
            }
            else
            {
                if (ALocationAnchorActor* Found = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths))
                {
                    TP.SourceAnchor = TSoftObjectPtr<UObject>(Found);
                    TP.SourceWorldPosition = Found->GetActorLocation();
                    TP.SourceWorldOrientation = Found->GetActorRotation();
                    bChanged = true;
                }
            }

            ResolveDestinationActorInTP(TP);

            if (bChanged) { ++Updated; bOwnerModified = true; }
        }

        if (bOwnerModified && OwnerAsset)
        {
            OwnerAsset->Modify();
            OwnerAsset->MarkPackageDirty();
        }
    };

    if (UFloorAsset* Floor = Cast<UFloorAsset>(Asset))
    {
        UpdateFromPoints(Floor->TransitionPoints, Floor);
    }
    else if (UInteriorSetAsset* Interior = Cast<UInteriorSetAsset>(Asset))
    {
        UpdateFromPoints(Interior->TransitionPoints, Interior);
    }
    else if (UStreetAsset* Street = Cast<UStreetAsset>(Asset))
    {
        UpdateFromPoints(Street->TransitionPoints, Street);
    }

    return Updated;
}

// Добавлена локальная функция поиска ассета этажа (используется для возможной миграции/добавления в ассет — опционально)
static UFloorAsset* FindFloorAssetByGuid(const FGuid& FloorGuid)
{
    if (!FloorGuid.IsValid()) return nullptr;
    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))

    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
        if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
        if (!IS) continue;

        for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
        {
            UFloorAsset* Floor = Ref.LoadSynchronous();
            if (!Floor) continue;
            if (Floor->FloorID == FloorGuid)
                return Floor;
        }
    }
    return nullptr;
}

static void ApplyFloorToComponent(
    UFloorAssignmentComponent* Comp,
    AActor* Owner,
    const FGuid& FloorGuid,
    const FGuid& InteriorSetGuid,
    const FText& ResFloorName,
    const FText& ResInteriorSetName)
{
    if (!Comp || !IsValid(Owner)) return;

    Owner->Modify();
    Comp->Modify();

    // Если у компонента ещё нет ItemId — создаём новый GUID и помечаем пакет dirty,
    // чтобы компонент можно было потом идентифицировать при сохранении/восстановлении.
    if (!Comp->ItemId.IsValid())
    {
#if WITH_EDITOR
        Comp->ItemId = FGuid::NewDeterministicGuid(Owner->GetPathName(), 0);
#else
        Comp->ItemId = FGuid::NewGuid();
#endif
    }

    Comp->FloorId         = FloorGuid;
    Comp->InteriorSetId   = InteriorSetGuid;
    Comp->FloorName       = ResFloorName;
    Comp->InteriorSetName = ResInteriorSetName;

#if WITH_EDITOR
    // Помечаем компонент как участника snapshot/restore
    Comp->SnapshotChannel = ESnapshotChannel::Snapshot;
#endif

    if (UPackage* Pkg = Owner->GetOutermost())
    {
        Pkg->MarkPackageDirty();
    }

#if WITH_EDITOR
    // Optionally persist ItemId in Floor asset (commented out by default).
    // UFloorAsset* FloorAsset = FindFloorAssetByGuid(FloorGuid);
    // if (FloorAsset && Comp->ItemId.IsValid()) { FloorAsset->Modify(); FloorAsset->AssignedItemIds.AddUnique(Comp->ItemId); FloorAsset->GetOutermost()->MarkPackageDirty(); }
#endif
}

int32 FFloorAssignerEditorLibrary::ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid)
{
    int32 ModifiedCount = 0;
    if (!GEditor) return 0;
    USelection* Selected = GEditor->GetSelectedActors();
    if (!Selected) return 0;

    // Begin editor undo transaction so action can be undone
    FScopedTransaction Transaction(NSLOCTEXT("FloorAssigner", "ApplyFloorToSelectedActors", "Assign floor to selected actors"));

    // Resolve display names for InteriorSet and Floor to write into components
    FText ResInteriorSetName = FText::GetEmpty();
    FText ResFloorName       = FText::GetEmpty();

    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
        if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
        if (!IS) continue;
        if (IS->InteriorSetID != InteriorSetGuid) continue;

        ResInteriorSetName = IS->DisplayName.IsEmpty() ? FText::FromName(IS->GetFName()) : IS->DisplayName;

        for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
        {
            UFloorAsset* Floor = Ref.LoadSynchronous();
            if (!Floor) continue;
            if (Floor->FloorID != FloorGuid) continue;

            ResFloorName = Floor->DisplayName.IsEmpty()
                ? FText::FromName(Floor->GetFName())
                : FText::Format(FText::FromString(TEXT("{0} — {1}")), FText::AsNumber(Floor->FloorIndex), Floor->DisplayName);
            break;
        }
        break;
    }

    TArray<AActor*> Actors;
    Selected->GetSelectedObjects<AActor>(Actors);

    // Collect unique packages to mark as dirty (so editor shows modified levels)
    TSet<UPackage*> PackagesToMark;

    for (AActor* A : Actors)
    {
        if (!IsValid(A)) continue;

        // Apply to the actor itself
        if (UFloorAssignmentComponent* Comp = A->FindComponentByClass<UFloorAssignmentComponent>())
        {
            ApplyFloorToComponent(Comp, A, FloorGuid, InteriorSetGuid, ResFloorName, ResInteriorSetName);
            if (UPackage* Pkg = A->GetOutermost()) PackagesToMark.Add(Pkg);
            ++ModifiedCount;
        }

        // Apply to child actors (ChildActorComponent) if they have FloorAssignmentComponent
        TArray<UChildActorComponent*> ChildActorComps;
        A->GetComponents<UChildActorComponent>(ChildActorComps);
        for (UChildActorComponent* ChildComp : ChildActorComps)
        {
            if (!IsValid(ChildComp)) continue;
            AActor* ChildActor = ChildComp->GetChildActor();
            if (!IsValid(ChildActor)) continue;

            if (UFloorAssignmentComponent* ChildFloorComp = ChildActor->FindComponentByClass<UFloorAssignmentComponent>())
            {
                ApplyFloorToComponent(ChildFloorComp, ChildActor, FloorGuid, InteriorSetGuid, ResFloorName, ResInteriorSetName);
                if (UPackage* CPkg = ChildActor->GetOutermost()) PackagesToMark.Add(CPkg);
                ++ModifiedCount;
            }
        }
    }

    // Mark all affected packages dirty so editor will show them as modified (user can save manually)
    for (UPackage* Pkg : PackagesToMark)
    {
        if (Pkg)
        {
            Pkg->SetDirtyFlag(true);
            Pkg->MarkPackageDirty();
        }
    }

    // Run validation for the floor (update transition points etc.)
    int32 ValidationUpdated = ValidateFloorTransitions(FloorGuid);
    UE_LOG(LogTemp, Log, TEXT("FloorAssignerEditorLibrary: Applied floor '%s' -> modified %d actors; validation updated %d entries"),
        *ResFloorName.ToString(), ModifiedCount, ValidationUpdated);

    FText Display = ResolveFloorDisplayName(FloorGuid);
    FMessageDialog::Open(EAppMsgType::Ok, FText::Format(FText::FromString(TEXT("Assigned '{0}' to {1} actors.")), Display, FText::AsNumber(ModifiedCount)));

    return ModifiedCount;
}

// -----------------------------------------------------------------------------
// Select actors by Floor GUID (editor-only)
// -----------------------------------------------------------------------------
int32 FFloorAssignerEditorLibrary::SelectActorsByFloor(const FGuid& FloorGuid)
{
    int32 Count = 0;
    if (!GEditor || !GEngine) return 0;

    // Clear previous selection
    GEditor->SelectNone(false, true, false);

    // Iterate only relevant worlds (Editor / PIE / Game) to avoid preview worlds
    const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
    for (const FWorldContext& WC : WorldContexts)
    {
        UWorld* World = WC.World();
        if (!World) continue;

        // Consider only Editor, PIE and Game worlds
        if (WC.WorldType != EWorldType::Editor && WC.WorldType != EWorldType::PIE && WC.WorldType != EWorldType::Game)
            continue;

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (!IsValid(Actor)) continue;
            UFloorAssignmentComponent* Comp = Actor->FindComponentByClass<UFloorAssignmentComponent>();
            if (!Comp) continue;

            // Select actors that are explicitly marked for snapshot (ignore FloorId here)
            if (Comp->SnapshotChannel != ESnapshotChannel::None)
            {
                GEditor->SelectActor(Actor, true, false, true);
                ++Count;
            }
        }
    }

    GEditor->NoteSelectionChange();

    FText Display = ResolveFloorDisplayName(FloorGuid);
    FMessageDialog::Open(EAppMsgType::Ok, FText::Format(FText::FromString(TEXT("Selected {0} actors for floor '{1}'")), FText::AsNumber(Count), Display));

    return Count;
}

// -----------------------------------------------------------------------------
// Unregister selected actors (editor-only)
// -----------------------------------------------------------------------------
int32 FFloorAssignerEditorLibrary::UnregisterSelectedActors()
{
    int32 Count = 0;
    if (!GEditor) return 0;

    USelection* Selected = GEditor->GetSelectedActors();
    if (!Selected) return 0;

    TArray<AActor*> Actors;
    Selected->GetSelectedObjects<AActor>(Actors);

    for (AActor* A : Actors)
    {
        if (!IsValid(A)) continue;

        UFloorAssignmentComponent* Comp = A->FindComponentByClass<UFloorAssignmentComponent>();
        if (!Comp) continue;

        Comp->Modify();
        A->Modify();

#if WITH_EDITOR
        Comp->SnapshotChannel = ESnapshotChannel::None;
#endif

        Comp->FloorId.Invalidate();
        Comp->FloorName = FText::GetEmpty();

        if (UPackage* Pkg = A->GetOutermost())
        {
            Pkg->MarkPackageDirty();
        }

        ++Count;
    }

    FText Msg = FText::Format(FText::FromString(TEXT("Unregistered and cleared floor for {0} selected actors.")), FText::AsNumber(Count));
    FMessageDialog::Open(EAppMsgType::Ok, Msg);

    return Count;
}

// -----------------------------------------------------------------------------
// Validation implementations (editor-only)
// -----------------------------------------------------------------------------
int32 FFloorAssignerEditorLibrary::ValidateStreetTransitions(const FGuid& StreetGuid)
{
    if (!StreetGuid.IsValid()) return 0;

    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("StreetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UStreetAsset* Street = Cast<UStreetAsset>(AD.GetAsset());
        if (!Street) Street = Cast<UStreetAsset>(Path.TryLoad());
        if (!Street) continue;
        if (Street->StreetID != StreetGuid) continue;

        TArray<FSoftObjectPath> LevelPaths;
        if (!Street->ParentWorldRegion.IsNull())
        {
            if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous())
            {
                if (!R->RegionLevel.IsNull())
                    LevelPaths.Add(R->RegionLevel.ToSoftObjectPath());
            }
        }

        // Scan region level(s) and interior/floor levels that belong to this street for unregistered anchors
        TArray<FSoftObjectPath> AllPaths = LevelPaths;
        for (const TSoftObjectPtr<UInteriorSetAsset>& IR : Street->InteriorSets)
            if (IR.IsValid())
                if (UInteriorSetAsset* IS2 = IR.LoadSynchronous())
                    for (const TSoftObjectPtr<UFloorAsset>& FR : IS2->Floors)
                        if (FR.IsValid()) AllPaths.Add(FR.ToSoftObjectPath());

        TArray<ALocationAnchorActor*> SceneAnchors;
        GetAnchorsInLevelPaths(AllPaths, SceneAnchors);
        TArray<FString> UnregisteredShort;
        for (ALocationAnchorActor* Anchor : SceneAnchors)
        {
            if (!IsValid(Anchor) || !Anchor->AnchorID.IsValid()) continue;
            bool bFound = false;
            for (const FLocationTransitionPoint& TP : Street->TransitionPoints) { if (TP.TransitionPointID == Anchor->AnchorID) { bFound = true; break; } }
            if (!bFound && Anchor->RequiresDestination)
            {
                FString RegionDisp;
                if (!Street->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous()) RegionDisp = R->DisplayName.ToString();
                UnregisteredShort.Add(FString::Printf(TEXT("%s (%s) — %s - %s"), *Anchor->GetName(), *Anchor->DisplayName.ToString(), *RegionDisp, *Street->DisplayName.ToString()));
                UE_LOG(LogTemp, Warning, TEXT("ValidateStreetTransitions: Anchor on level not registered in Street asset: Actor=%s, Asset=%s, Pos=%s"), *Anchor->GetName(), *Street->GetName(), *Anchor->GetActorLocation().ToString());
            }
            
        }
        if (UnregisteredShort.Num() > 0)
        {
            FString Msg;
            for (const FString& S : UnregisteredShort) { Msg += S + TEXT("\n"); }
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Found %d anchors on scene not registered in Street asset:\n%s"), UnregisteredShort.Num(), *Msg)));
        }

        int32 Removed = ULocationEditorUtils::ValidateAndCleanTransitionPoints(Street);
        int32 Updated = UpdateTransitionPointsForAsset(Street, LevelPaths);

        // Reciprocal link checks
        TArray<FString> ShortMessages;
        for (const FLocationTransitionPoint& TP : Street->TransitionPoints)
        {
            if (!TP.DestinationLink.TargetAnchorID.IsValid()) continue;

            ALocationAnchorActor* SourceActor = nullptr;
            if (!TP.SourceAnchor.IsNull()) SourceActor = Cast<ALocationAnchorActor>(TP.SourceAnchor.Get());
            if (!SourceActor) SourceActor = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths);

            // Resolve dest level
            FSoftObjectPath DestLevelPath;
            if (!TP.DestinationLink.TargetFloor.IsNull())
            {
                if (UFloorAsset* F = TP.DestinationLink.TargetFloor.LoadSynchronous())
                    if (!F->FloorLevel.IsNull()) DestLevelPath = F->FloorLevel.ToSoftObjectPath();
            }
            if (!DestLevelPath.IsValid() && !TP.DestinationLink.TargetRegion.IsNull())
            {
                if (UWorldRegionAsset* R = TP.DestinationLink.TargetRegion.LoadSynchronous())
                    if (!R->RegionLevel.IsNull()) DestLevelPath = R->RegionLevel.ToSoftObjectPath();
            }

            ALocationAnchorActor* DestActor = nullptr;
            if (DestLevelPath.IsValid()) DestActor = FindAnchorOnLevel(TP.DestinationLink.TargetAnchorID, DestLevelPath);

            bool bReciprocal = false;
            if (DestActor)
            {
                if (DestActor->DestinationLink.TargetAnchorID == TP.TransitionPointID) bReciprocal = true;
                else if (UObject* DAReg = DestActor->GetOwnerRegistrationAsset())
                {
                    if (UWorldRegionAsset* Reg = Cast<UWorldRegionAsset>(DAReg))
                    {
                        for (const FLocationAnchor& A : Reg->Anchors) if (A.AnchorID == DestActor->AnchorID && A.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                    }
                    else if (UStreetAsset* S2 = Cast<UStreetAsset>(DAReg))
                    {
                        for (const FLocationAnchor& A : S2->Anchors) if (A.AnchorID == DestActor->AnchorID && A.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                    }
                    else if (UInteriorSetAsset* IS2 = Cast<UInteriorSetAsset>(DAReg))
                    {
                        for (const TSoftObjectPtr<UFloorAsset>& FloorRef : IS2->Floors)
                        {
                            if (!FloorRef.IsValid()) continue;
                            UFloorAsset* FloorObj = FloorRef.LoadSynchronous();
                            if (!FloorObj) continue;
                            for (const FLocationAnchor& AnchorEntry : FloorObj->Anchors)
                            {
                                if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID)
                                {
                                    bReciprocal = true;
                                    break;
                                }
                            }
                            if (bReciprocal) break;
                        }
                    }
                    else if (UFloorAsset* F2 = Cast<UFloorAsset>(DAReg))
                    {
                        for (const FLocationAnchor& A : F2->Anchors) if (A.AnchorID == DestActor->AnchorID && A.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                    }
                }
            }

            if (!bReciprocal)
            {
                FString ObjName = SourceActor ? SourceActor->GetName() : TEXT("<no actor>");
                FString Disp = SourceActor ? SourceActor->DisplayName.ToString() : TP.DisplayName.ToString();
                FString RegionDisp, StreetDisp;
                if (!Street->ParentWorldRegion.IsNull()) { if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous()) RegionDisp = R->DisplayName.ToString(); }
                StreetDisp = Street->DisplayName.ToString();
                FString Loc = FString::Printf(TEXT("%s - %s"), *RegionDisp, *StreetDisp);
                ShortMessages.Add(FString::Printf(TEXT("%s (%s) — %s"), *ObjName, *Disp, *Loc));

                FVector Pos = SourceActor ? SourceActor->GetActorLocation() : TP.SourceWorldPosition;
                FString AssetNames = Street->GetName();
                if (!Street->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = Street->ParentWorldRegion.LoadSynchronous()) AssetNames = FString::Printf(TEXT("%s/%s"), *R->GetName(), *Street->GetName());
                UE_LOG(LogTemp, Warning, TEXT("ValidateStreetTransitions: Non-reciprocal anchor '%s' (%s) in street asset '%s' at pos %s -> dest anchor id %s"),
                    *ObjName, *Disp, *AssetNames, *Pos.ToString(), *TP.DestinationLink.TargetAnchorID.ToString());
            }
        }

        if (ShortMessages.Num() > 0)
        {
            FString Msg;
            for (const FString& S : ShortMessages) { Msg += S + TEXT("\n"); }
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg));
        }

        UE_LOG(LogTemp, Log, TEXT("ValidateStreetTransitions: Street '%s' removed %d, updated %d transition points"), *Street->GetName(), Removed, Updated);
        return Removed + Updated;
    }

    UE_LOG(LogTemp, Warning, TEXT("ValidateStreetTransitions: Street with GUID %s not found"), *StreetGuid.ToString());
    return 0;
}

int32 FFloorAssignerEditorLibrary::ValidateInteriorSetTransitions(const FGuid& InteriorSetGuid)
{
    if (!InteriorSetGuid.IsValid()) return 0;

    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
        if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
        if (!IS) continue;
        if (IS->InteriorSetID != InteriorSetGuid) continue;

        TArray<FSoftObjectPath> LevelPaths;
        if (!IS->ParentStreet.IsNull())
        {
            if (UStreetAsset* S = IS->ParentStreet.LoadSynchronous())
            {
                if (!S->ParentWorldRegion.IsNull())
                {
                    if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous())
                    {
                        if (!R->RegionLevel.IsNull())
                            LevelPaths.Add(R->RegionLevel.ToSoftObjectPath());
                    }
                }
            }
        }

        // For interior set, gather the region level and floors' levels; floors have separate scenes
        TArray<FSoftObjectPath> AllPaths = LevelPaths;
        for (const TSoftObjectPtr<UFloorAsset>& FR : IS->Floors) if (FR.IsValid()) AllPaths.Add(FR.ToSoftObjectPath());

        TArray<ALocationAnchorActor*> SceneAnchors;
        GetAnchorsInLevelPaths(AllPaths, SceneAnchors);
        TArray<FString> UnregisteredShort;
        for (ALocationAnchorActor* Anchor : SceneAnchors)
        {
            if (!IsValid(Anchor) || !Anchor->AnchorID.IsValid()) continue;
            bool bFound = false;
            for (const FLocationTransitionPoint& TP : IS->TransitionPoints) { if (TP.TransitionPointID == Anchor->AnchorID) { bFound = true; break; } }
            if (!bFound && Anchor->RequiresDestination)
            {
                FString RegionDisp, StreetDisp;
                if (!IS->ParentStreet.IsNull()) if (UStreetAsset* S = IS->ParentStreet.LoadSynchronous()) { StreetDisp = S->DisplayName.ToString(); if (!S->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous()) RegionDisp = R->DisplayName.ToString(); }
                UnregisteredShort.Add(FString::Printf(TEXT("%s (%s) — %s - %s - %s"), *Anchor->GetName(), *Anchor->DisplayName.ToString(), *RegionDisp, *StreetDisp, *IS->DisplayName.ToString()));
                UE_LOG(LogTemp, Warning, TEXT("ValidateInteriorSetTransitions: Anchor on level not registered in InteriorSet asset: Actor=%s, Asset=%s, Pos=%s"), *Anchor->GetName(), *IS->GetName(), *Anchor->GetActorLocation().ToString());
            }
            
        }
        if (UnregisteredShort.Num() > 0)
        {
            FString Msg;
            for (const FString& S : UnregisteredShort) { Msg += S + TEXT("\n"); }
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Found %d anchors on scene not registered in InteriorSet asset:\n%s"), UnregisteredShort.Num(), *Msg)));
        }

        int32 Removed = ULocationEditorUtils::ValidateAndCleanTransitionPoints(IS);
        int32 Updated = UpdateTransitionPointsForAsset(IS, LevelPaths);

        // Reciprocal check
        TArray<FString> ShortMessages;
        for (const FLocationTransitionPoint& TP : IS->TransitionPoints)
        {
            if (!TP.DestinationLink.TargetAnchorID.IsValid()) continue;

            ALocationAnchorActor* SourceActor = nullptr;
            if (!TP.SourceAnchor.IsNull()) SourceActor = Cast<ALocationAnchorActor>(TP.SourceAnchor.Get());
            if (!SourceActor) SourceActor = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths);

            FSoftObjectPath DestLevelPath;
            if (!TP.DestinationLink.TargetFloor.IsNull())
            {
                if (UFloorAsset* F = TP.DestinationLink.TargetFloor.LoadSynchronous()) if (!F->FloorLevel.IsNull()) DestLevelPath = F->FloorLevel.ToSoftObjectPath();
            }
            if (!DestLevelPath.IsValid() && !TP.DestinationLink.TargetRegion.IsNull())
            {
                if (UWorldRegionAsset* R = TP.DestinationLink.TargetRegion.LoadSynchronous()) if (!R->RegionLevel.IsNull()) DestLevelPath = R->RegionLevel.ToSoftObjectPath();
            }

            ALocationAnchorActor* DestActor = nullptr;
            if (DestLevelPath.IsValid()) DestActor = FindAnchorOnLevel(TP.DestinationLink.TargetAnchorID, DestLevelPath);

            bool bReciprocal = false;
            if (DestActor)
            {
                if (DestActor->DestinationLink.TargetAnchorID == TP.TransitionPointID)
                {
                    bReciprocal = true;
                }
                else if (UObject* DAReg = DestActor->GetOwnerRegistrationAsset())
                {
                    if (UWorldRegionAsset* Reg = Cast<UWorldRegionAsset>(DAReg))
                    {
                        for (const FLocationAnchor& AnchorEntry : Reg->Anchors)
                        {
                            if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                        }
                    }
                    else if (UStreetAsset* StreetOwner = Cast<UStreetAsset>(DAReg))
                    {
                        for (const FLocationAnchor& AnchorEntry : StreetOwner->Anchors)
                        {
                            if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                        }
                    }
                    else if (UInteriorSetAsset* InteriorOwner = Cast<UInteriorSetAsset>(DAReg))
                    {
                        for (const TSoftObjectPtr<UFloorAsset>& FloorRef : InteriorOwner->Floors)
                        {
                            if (!FloorRef.IsValid()) continue;
                            UFloorAsset* FloorObj = FloorRef.LoadSynchronous();
                            if (!FloorObj) continue;
                            for (const FLocationAnchor& AnchorEntry : FloorObj->Anchors)
                            {
                                if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                            }
                            if (bReciprocal) break;
                        }
                    }
                    else if (UFloorAsset* FloorOwner = Cast<UFloorAsset>(DAReg))
                    {
                        for (const FLocationAnchor& AnchorEntry : FloorOwner->Anchors)
                        {
                            if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                        }
                    }
                }
            }

            if (!bReciprocal)
            {
                FString ObjName = SourceActor ? SourceActor->GetName() : TEXT("<no actor>");
                FString Disp = SourceActor ? SourceActor->DisplayName.ToString() : TP.DisplayName.ToString();
                FString RegionDisp, StreetDisp, ISDisp;
                if (!IS->ParentStreet.IsNull()) if (UStreetAsset* S = IS->ParentStreet.LoadSynchronous()) { StreetDisp = S->DisplayName.ToString(); if (!S->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous()) RegionDisp = R->DisplayName.ToString(); }
                ISDisp = IS->DisplayName.ToString();
                FString Loc = FString::Printf(TEXT("%s - %s - %s"), *RegionDisp, *StreetDisp, *ISDisp);
                ShortMessages.Add(FString::Printf(TEXT("%s (%s) — %s"), *ObjName, *Disp, *Loc));

                FVector Pos = SourceActor ? SourceActor->GetActorLocation() : TP.SourceWorldPosition;
                FString AssetNames = IS->GetName();
                if (!IS->ParentStreet.IsNull()) if (UStreetAsset* S = IS->ParentStreet.LoadSynchronous()) { if (!S->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous()) AssetNames = FString::Printf(TEXT("%s/%s/%s"), *R->GetName(), *S->GetName(), *IS->GetName()); else AssetNames = FString::Printf(TEXT("%s/%s"), *S->GetName(), *IS->GetName()); }
                UE_LOG(LogTemp, Warning, TEXT("ValidateInteriorSetTransitions: Non-reciprocal anchor '%s' (%s) in interiorset asset '%s' at pos %s -> dest anchor id %s"),
                    *ObjName, *Disp, *AssetNames, *Pos.ToString(), *TP.DestinationLink.TargetAnchorID.ToString());
            }
        }

        if (ShortMessages.Num() > 0)
        {
            FString Msg;
            for (const FString& S : ShortMessages) { Msg += S + TEXT("\n"); }
            FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg));
        }

        UE_LOG(LogTemp, Log, TEXT("ValidateInteriorSetTransitions: InteriorSet '%s' removed %d, updated %d transition points"), *IS->GetName(), Removed, Updated);
        return Removed + Updated;
    }

    UE_LOG(LogTemp, Warning, TEXT("ValidateInteriorSetTransitions: InteriorSet with GUID %s not found"), *InteriorSetGuid.ToString());
    return 0;
}

int32 FFloorAssignerEditorLibrary::ValidateFloorTransitions(const FGuid& FloorGuid)
{
    if (!FloorGuid.IsValid()) return 0;

    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
        if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
        if (!IS) continue;

        for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
        {
            if (!Ref.IsValid()) continue;
            UFloorAsset* Floor = Ref.LoadSynchronous();
            if (!Floor) continue;
            if (Floor->FloorID != FloorGuid) continue;

            TArray<FSoftObjectPath> LevelPaths;
            if (!Floor->FloorLevel.IsNull()) LevelPaths.Add(Floor->FloorLevel.ToSoftObjectPath());

            // For floor, check anchors on its level
            TArray<ALocationAnchorActor*> SceneAnchors;
            GetAnchorsOnLevel(Floor->FloorLevel.ToSoftObjectPath(), SceneAnchors);
            TArray<FString> UnregisteredShort;
            for (ALocationAnchorActor* Anchor : SceneAnchors)
            {
                if (!IsValid(Anchor) || !Anchor->AnchorID.IsValid()) continue;
                bool bFound = false;
                for (const FLocationTransitionPoint& TP : Floor->TransitionPoints) { if (TP.TransitionPointID == Anchor->AnchorID) { bFound = true; break; } }
                if (!bFound && Anchor->RequiresDestination)
                {
                    FString RegionDisp, StreetDisp, ISDisp;
                    if (UInteriorSetAsset* PIS = Floor->ParentInteriorSet.LoadSynchronous()) { ISDisp = PIS->DisplayName.ToString(); if (!PIS->ParentStreet.IsNull()) if (UStreetAsset* S = PIS->ParentStreet.LoadSynchronous()) { StreetDisp = S->DisplayName.ToString(); if (!S->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous()) RegionDisp = R->DisplayName.ToString(); } }
                    UnregisteredShort.Add(FString::Printf(TEXT("%s (%s) — %s - %s - %s - %s"), *Anchor->GetName(), *Anchor->DisplayName.ToString(), *RegionDisp, *StreetDisp, *ISDisp, *Floor->DisplayName.ToString()));
                    UE_LOG(LogTemp, Warning, TEXT("ValidateFloorTransitions: Anchor on level not registered in Floor asset: Actor=%s, Asset=%s, Pos=%s"), *Anchor->GetName(), *Floor->GetName(), *Anchor->GetActorLocation().ToString());
                }
                
            }
            if (UnregisteredShort.Num() > 0)
            {
                FString Msg;
                for (const FString& S : UnregisteredShort) { Msg += S + TEXT("\n"); }
                FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Found %d anchors on scene not registered in Floor asset:\n%s"), UnregisteredShort.Num(), *Msg)));
            }

            int32 Removed = ULocationEditorUtils::ValidateAndCleanTransitionPoints(Floor);
            int32 Updated = UpdateTransitionPointsForAsset(Floor, LevelPaths);

            // Reciprocal checks for floor
            TArray<FString> ShortMessages;
            for (const FLocationTransitionPoint& TP : Floor->TransitionPoints)
            {
                if (!TP.DestinationLink.TargetAnchorID.IsValid()) continue;

                ALocationAnchorActor* SourceActor = nullptr;
                if (!TP.SourceAnchor.IsNull()) SourceActor = Cast<ALocationAnchorActor>(TP.SourceAnchor.Get());
                if (!SourceActor) SourceActor = FindAnchorInLevelPaths(TP.TransitionPointID, LevelPaths);

                FSoftObjectPath DestLevelPath;
                if (!TP.DestinationLink.TargetFloor.IsNull())
                {
                    if (UFloorAsset* F = TP.DestinationLink.TargetFloor.LoadSynchronous()) if (!F->FloorLevel.IsNull()) DestLevelPath = F->FloorLevel.ToSoftObjectPath();
                }
                if (!DestLevelPath.IsValid() && !TP.DestinationLink.TargetRegion.IsNull())
                {
                    if (UWorldRegionAsset* R = TP.DestinationLink.TargetRegion.LoadSynchronous()) if (!R->RegionLevel.IsNull()) DestLevelPath = R->RegionLevel.ToSoftObjectPath();
                }

                ALocationAnchorActor* DestActor = nullptr;
                if (DestLevelPath.IsValid()) DestActor = FindAnchorOnLevel(TP.DestinationLink.TargetAnchorID, DestLevelPath);

                bool bReciprocal = false;
                if (DestActor)
                {
                    if (DestActor->DestinationLink.TargetAnchorID == TP.TransitionPointID)
                    {
                        bReciprocal = true;
                    }
                    else if (UObject* DAReg = DestActor->GetOwnerRegistrationAsset())
                    {
                        if (UWorldRegionAsset* Reg = Cast<UWorldRegionAsset>(DAReg))
                        {
                            for (const FLocationAnchor& AnchorEntry : Reg->Anchors) if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                        }
                        else if (UStreetAsset* StreetOwner = Cast<UStreetAsset>(DAReg))
                        {
                            for (const FLocationAnchor& AnchorEntry : StreetOwner->Anchors) if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                        }
                        else if (UInteriorSetAsset* InteriorOwner = Cast<UInteriorSetAsset>(DAReg))
                        {
                            for (const TSoftObjectPtr<UFloorAsset>& FloorRef : InteriorOwner->Floors)
                            {
                                if (!FloorRef.IsValid()) continue;
                                UFloorAsset* FloorObj = FloorRef.LoadSynchronous();
                                if (!FloorObj) continue;
                                for (const FLocationAnchor& AnchorEntry : FloorObj->Anchors) if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                                if (bReciprocal) break;
                            }
                        }
                        else if (UFloorAsset* FloorOwner = Cast<UFloorAsset>(DAReg))
                        {
                            for (const FLocationAnchor& AnchorEntry : FloorOwner->Anchors) if (AnchorEntry.AnchorID == DestActor->AnchorID && AnchorEntry.ReturnLink.TargetAnchorID == TP.TransitionPointID) { bReciprocal = true; break; }
                        }
                    }
                }

                if (!bReciprocal)
                {
                    FString ObjName = SourceActor ? SourceActor->GetName() : TEXT("<no actor>");
                    FString Disp = SourceActor ? SourceActor->DisplayName.ToString() : TP.DisplayName.ToString();
                    FString RegionDisp, StreetDisp, ISDisp, FloorDisp;
                    if (UInteriorSetAsset* PIS = Floor->ParentInteriorSet.LoadSynchronous())
                    {
                        ISDisp = PIS->DisplayName.ToString();
                        if (!PIS->ParentStreet.IsNull()) if (UStreetAsset* S = PIS->ParentStreet.LoadSynchronous()) { StreetDisp = S->DisplayName.ToString(); if (!S->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous()) RegionDisp = R->DisplayName.ToString(); }
                    }
                    FloorDisp = Floor->DisplayName.ToString();
                    FString Loc = FString::Printf(TEXT("%s - %s - %s - %s"), *RegionDisp, *StreetDisp, *ISDisp, *FloorDisp);
                    ShortMessages.Add(FString::Printf(TEXT("%s (%s) — %s"), *ObjName, *Disp, *Loc));

                    FVector Pos = SourceActor ? SourceActor->GetActorLocation() : TP.SourceWorldPosition;
                    FString AssetNames = Floor->GetName();
                    if (UInteriorSetAsset* PIS2 = Floor->ParentInteriorSet.LoadSynchronous()) if (!PIS2->ParentStreet.IsNull()) if (UStreetAsset* S = PIS2->ParentStreet.LoadSynchronous()) if (!S->ParentWorldRegion.IsNull()) if (UWorldRegionAsset* R = S->ParentWorldRegion.LoadSynchronous()) AssetNames = FString::Printf(TEXT("%s/%s/%s/%s"), *R->GetName(), *S->GetName(), *PIS2->GetName(), *Floor->GetName());
                    UE_LOG(LogTemp, Warning, TEXT("ValidateFloorTransitions: Non-reciprocal anchor '%s' (%s) in floor asset '%s' at pos %s -> dest anchor id %s"),
                        *ObjName, *Disp, *AssetNames, *Pos.ToString(), *TP.DestinationLink.TargetAnchorID.ToString());
                }
            }

            if (ShortMessages.Num() > 0)
            {
                FString Msg;
                for (const FString& S : ShortMessages) { Msg += S + TEXT("\n"); }
                FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg));
            }

            UE_LOG(LogTemp, Log, TEXT("ValidateFloorTransitions: Floor '%s' removed %d, updated %d transition points"), *Floor->GetName(), Removed, Updated);
            return Removed + Updated;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("ValidateFloorTransitions: Floor with GUID %s not found"), *FloorGuid.ToString());
    return 0;
}

// -----------------------------------------------------------------------------
// Getters: WorldMaps / Regions / Streets / InteriorSets / Floors
// -----------------------------------------------------------------------------
// Implementations mirror previous runtime versions

TMap<FGuid, FText> FFloorAssignerEditorLibrary::GetWorldMaps()
{
    TMap<FGuid, FText> Out;
    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldMapAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UWorldMapAsset* Map = Cast<UWorldMapAsset>(AD.GetAsset());
        if (!Map) Map = Cast<UWorldMapAsset>(Path.TryLoad());
        if (Map && Map->WorldMapID.IsValid())
            Out.Add(Map->WorldMapID, Map->DisplayName.IsEmpty() ? FText::FromName(Map->GetFName()) : Map->DisplayName);
    }
    return Out;
}

TMap<FGuid, FText> FFloorAssignerEditorLibrary::GetRegions(const FGuid& WorldMapGuid)
{
    TMap<FGuid, FText> Out;
    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldMapAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UWorldMapAsset* Map = Cast<UWorldMapAsset>(AD.GetAsset());
        if (!Map) Map = Cast<UWorldMapAsset>(Path.TryLoad());
        if (!Map) continue;
        if (Map->WorldMapID != WorldMapGuid) continue;

        for (const TSoftObjectPtr<UWorldRegionAsset>& Ref : Map->Regions)
        {
            UWorldRegionAsset* Region = Ref.LoadSynchronous();
            if (!Region) continue;
            if (!Region->WorldRegionID.IsValid()) continue;
            Out.Add(Region->WorldRegionID, Region->DisplayName.IsEmpty() ? FText::FromName(Region->GetFName()) : Region->DisplayName);
        }
        break;
    }
    return Out;
}

TMap<FGuid, FText> FFloorAssignerEditorLibrary::GetStreets(const FGuid& WorldRegionGuid)
{
    TMap<FGuid, FText> Out;
    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("WorldRegionAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UWorldRegionAsset* Region = Cast<UWorldRegionAsset>(AD.GetAsset());
        if (!Region) Region = Cast<UWorldRegionAsset>(Path.TryLoad());
        if (!Region) continue;
        if (Region->WorldRegionID != WorldRegionGuid) continue;

        for (const TSoftObjectPtr<UStreetAsset>& Ref : Region->Streets)
        {
            UStreetAsset* Street = Ref.LoadSynchronous();
            if (!Street) continue;
            if (!Street->StreetID.IsValid()) continue;
            Out.Add(Street->StreetID, Street->DisplayName.IsEmpty() ? FText::FromName(Street->GetFName()) : Street->DisplayName);
        }
        break;
    }
    return Out;
}

TMap<FGuid, FText> FFloorAssignerEditorLibrary::GetInteriorSets(const FGuid& StreetGuid)
{
    TMap<FGuid, FText> Out;
    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("StreetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UStreetAsset* Street = Cast<UStreetAsset>(AD.GetAsset());
        if (!Street) Street = Cast<UStreetAsset>(Path.TryLoad());
        if (!Street) continue;
        if (Street->StreetID != StreetGuid) continue;

        for (const TSoftObjectPtr<UInteriorSetAsset>& Ref : Street->InteriorSets)
        {
            UInteriorSetAsset* IS = Ref.LoadSynchronous();
            if (!IS) continue;
            if (!IS->InteriorSetID.IsValid()) continue;
            Out.Add(IS->InteriorSetID, IS->DisplayName.IsEmpty() ? FText::FromName(IS->GetFName()) : IS->DisplayName);
        }
        break;
    }
    return Out;
}

TMap<FGuid, FText> FFloorAssignerEditorLibrary::GetFloors(const FGuid& InteriorSetGuid)
{
    TMap<FGuid, FText> Out;
    for (const FAssetData& AD : GetAssetDataByClassName(TEXT("InteriorSetAsset")))
    {
        FSoftObjectPath Path = AD.ToSoftObjectPath();
        UInteriorSetAsset* IS = Cast<UInteriorSetAsset>(AD.GetAsset());
        if (!IS) IS = Cast<UInteriorSetAsset>(Path.TryLoad());
        if (!IS) continue;
        if (IS->InteriorSetID != InteriorSetGuid) continue;

        for (const TSoftObjectPtr<UFloorAsset>& Ref : IS->Floors)
        {
            UFloorAsset* Floor = Ref.LoadSynchronous();
            if (!Floor) continue;
            if (!Floor->FloorID.IsValid()) continue;
            Out.Add(Floor->FloorID, Floor->DisplayName.IsEmpty() ? FText::FromName(Floor->GetFName()) : Floor->DisplayName);
        }
        break;
    }
    return Out;
}

#endif // WITH_EDITOR