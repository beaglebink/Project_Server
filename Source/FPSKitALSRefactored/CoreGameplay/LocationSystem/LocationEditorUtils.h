#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LocationEditorUtils.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

UCLASS()
class FPSKITALSREFACTORED_API ULocationEditorUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
#endif
    static int32 AutoFillParentsForWorldMap(UWorldMapAsset* WorldMap);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
#endif
    static int32 AutoFillParentsForStreet(UStreetAsset* Street);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
#endif
    static int32 AutoFillParentsForInteriorSet(UInteriorSetAsset* InteriorSet);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Query")
#endif
    static TArray<UWorldMapAsset*> GetAllWorldMapAssets();

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static TMap<FGuid, FText> GetWorldMaps();

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static TMap<FGuid, FText> GetRegions(const FGuid& WorldMapGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static TMap<FGuid, FText> GetStreets(const FGuid& WorldRegionGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static TMap<FGuid, FText> GetInteriorSets(const FGuid& StreetGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static TMap<FGuid, FText> GetFloors(const FGuid& InteriorSetGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static int32 ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static int32 SelectActorsByFloor(const FGuid& FloorGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
#endif
    static int32 UnregisterSelectedActors();

    // ── Якоря ──────────────────────────────────────────────────────────────

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static TMap<FGuid, FText> GetRegionAnchors(UWorldRegionAsset* Region);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static TMap<FGuid, FText> GetFloorAnchors(UFloorAsset* Floor);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static int32 SyncAllAnchorsOnMap();

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static bool EstablishBidirectionalLink(
        UWorldRegionAsset* SourceRegion,
        const FGuid& SourceAnchorID,
        UFloorAsset* DestFloor,
        const FGuid& DestAnchorID);

    // --- Transition points management ---
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static bool RegisterTransitionPoint(class ALocationAnchorActor* SourceAnchor);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static int32 ValidateAndCleanTransitionPoints(UObject* Asset);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static int32 ValidateAllTransitionPoints();

    /**
     * Сканирует ВСЕ ассеты (WorldRegionAsset, StreetAsset, InteriorSetAsset, FloorAsset)
     * через AssetRegistry и удаляет из них FLocationTransitionPoint с TransitionPointID == AnchorID.
     * Используется при смене «адреса» актора перед регистрацией в новом ассете.
     * @return Количество удалённых записей.
     */
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
#endif
    static int32 RemoveTransitionPointFromAllAssets(const FGuid& AnchorID);

    // Blueprint-callable validation wrappers (call from Blutility)
#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Validation")
#endif
    static int32 ValidateStreetTransitionsByGuid(const FGuid& StreetGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Validation")
#endif
    static int32 ValidateInteriorSetTransitionsByGuid(const FGuid& InteriorSetGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Validation")
#endif
    static int32 ValidateFloorTransitionsByGuid(const FGuid& FloorGuid);

#if WITH_EDITOR
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Validation")
#endif
    static int32 ValidateRegionTransitionsByGuid(const FGuid& RegionGuid);
};