#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

class UFloorAssignmentComponent;

/**
 * Editor-only utilities for floor assignment.
 * Plain C++ — не UCLASS, не виден Blueprint-ам в Shipping.
 */
class FPSKITALSREFACTORED_API FFloorAssignerEditorLibrary
{
public:
    static TMap<FGuid, FText> GetWorldMaps();
    static int32 UnregisterSelectedActors();
    static int32 SelectActorsByFloor(const FGuid& FloorGuid);
    static TMap<FGuid, FText> GetRegions(const FGuid& WorldMapGuid);
    static TMap<FGuid, FText> GetStreets(const FGuid& WorldRegionGuid);
    static TMap<FGuid, FText> GetInteriorSets(const FGuid& StreetGuid);
    static TMap<FGuid, FText> GetFloors(const FGuid& InteriorSetGuid);
    static int32 ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid);

    static int32 ValidateStreetTransitions(const FGuid& StreetGuid);
    static int32 ValidateRegionTransitions(const FGuid& RegionGuid);
    static int32 ValidateInteriorSetTransitions(const FGuid& InteriorSetGuid);
    static int32 ValidateFloorTransitions(const FGuid& FloorGuid);
};

#endif // WITH_EDITOR