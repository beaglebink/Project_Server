#include "LocationEditorUtils.h"
#include "WorldRegionAsset.h"
#include "FloorAsset.h"
#include "LocationAnchorActor.h"
#include "EngineUtils.h"
#include "Editor.h"

#if WITH_EDITOR

TMap<FGuid, FText> ULocationEditorUtils::GetRegionAnchors(UWorldRegionAsset* Region)
{
    TMap<FGuid, FText> Result;
    if (!Region) return Result;

    for (const FLocationAnchor& A : Region->Anchors)
    {
        Result.Add(A.AnchorID, A.DisplayName);
    }
    return Result;
}

TMap<FGuid, FText> ULocationEditorUtils::GetFloorAnchors(UFloorAsset* Floor)
{
    TMap<FGuid, FText> Result;
    if (!Floor) return Result;

    for (const FLocationAnchor& A : Floor->Anchors)
    {
        Result.Add(A.AnchorID, A.DisplayName);
    }
    return Result;
}

int32 ULocationEditorUtils::SyncAllAnchorsOnMap()
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return 0;

    int32 Count = 0;
    for (TActorIterator<ALocationAnchorActor> It(World); It; ++It)
    {
        It->SyncToAsset();
        It->RefreshLinkDisplayName();
        ++Count;
    }
    return Count;
}

bool ULocationEditorUtils::EstablishBidirectionalLink(
    UWorldRegionAsset* SourceRegion,
    const FGuid& SourceAnchorID,
    UFloorAsset* DestFloor,
    const FGuid& DestAnchorID)
{
    if (!SourceRegion || !DestFloor) return false;

    // Найти исходный якорь в регионе и прописать ему DestinationLink → этаж
    FLocationAnchor* SrcAnchor = nullptr;
    for (FLocationAnchor& A : SourceRegion->Anchors)
    {
        if (A.AnchorID == SourceAnchorID) { SrcAnchor = &A; break; }
    }

    // Найти целевой якорь на этаже и прописать ему ReturnLink → регион
    FLocationAnchor* DstAnchor = nullptr;
    for (FLocationAnchor& A : DestFloor->Anchors)
    {
        if (A.AnchorID == DestAnchorID) { DstAnchor = &A; break; }
    }

    if (!SrcAnchor || !DstAnchor) return false;

    // Прямая связь: регион → этаж
    SrcAnchor->ReturnLink.TargetFloor = DestFloor;
    SrcAnchor->ReturnLink.TargetAnchorID = DestAnchorID;
    SrcAnchor->ReturnLink.TargetAnchorDisplayName = DstAnchor->DisplayName;

    // Обратная связь: этаж → регион
    DstAnchor->ReturnLink.TargetRegion = SourceRegion;
    DstAnchor->ReturnLink.TargetAnchorID = SourceAnchorID;
    DstAnchor->ReturnLink.TargetAnchorDisplayName = SrcAnchor->DisplayName;

    SourceRegion->MarkPackageDirty();
    DestFloor->MarkPackageDirty();

    return true;
}

#endif