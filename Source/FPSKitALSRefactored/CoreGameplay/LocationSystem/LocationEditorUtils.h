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
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
    static int32 AutoFillParentsForWorldMap(UWorldMapAsset* WorldMap);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
    static int32 AutoFillParentsForStreet(UStreetAsset* Street);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationSystem|Editor")
    static int32 AutoFillParentsForInteriorSet(UInteriorSetAsset* InteriorSet);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "LocationEditor|Query")
    static TArray<UWorldMapAsset*> GetAllWorldMapAssets();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static TMap<FGuid, FText> GetWorldMaps();

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static TMap<FGuid, FText> GetRegions(const FGuid& WorldMapGuid);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static TMap<FGuid, FText> GetStreets(const FGuid& WorldRegionGuid);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static TMap<FGuid, FText> GetInteriorSets(const FGuid& StreetGuid);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static TMap<FGuid, FText> GetFloors(const FGuid& InteriorSetGuid);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static int32 ApplyFloorToSelectedActors(const FGuid& FloorGuid, const FGuid& InteriorSetGuid);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static int32 SelectActorsByFloor(const FGuid& FloorGuid);

    UFUNCTION(BlueprintCallable, CallInEditor, Category = "FloorAssigner")
    static int32 UnregisterSelectedActors();

    // ── Якоря ──────────────────────────────────────────────────────────────

    /** Возвращает список якорей региона: AnchorID → DisplayName */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static TMap<FGuid, FText> GetRegionAnchors(UWorldRegionAsset* Region);

    /** Возвращает список якорей этажа: AnchorID → DisplayName */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static TMap<FGuid, FText> GetFloorAnchors(UFloorAsset* Floor);

    /** Синхронизирует все ALocationAnchorActor на текущей карте с их ассетами */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static int32 SyncAllAnchorsOnMap();

    /** Создаёт двустороннюю связь: находит целевой якорь в ассете и прописывает ему ReturnLink обратно */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static bool EstablishBidirectionalLink(
        UWorldRegionAsset* SourceRegion,
        const FGuid& SourceAnchorID,
        UFloorAsset* DestFloor,
        const FGuid& DestAnchorID);

    // --- New: transition points management ---
    /** Регистрирует/обновляет TransitionPoint для указанного актора (Editor only). */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static bool RegisterTransitionPoint(class ALocationAnchorActor* SourceAnchor);

    /** Проверяет и очищает устаревшие TransitionPoints в ассете (Editor only). Возвращает число удалённых записей. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static int32 ValidateAndCleanTransitionPoints(UObject* Asset);

    /** Проверяет все Street/InteriorSet/Floor ассеты в проекте и очищает устаревшие записи. */
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "AnchorEditor")
    static int32 ValidateAllTransitionPoints();
};