#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LocationEditorUtils.generated.h"

class UWorldMapAsset;
class UStreetAsset;
class UInteriorSetAsset;

/**
 * Editor utility functions — видны в Blueprint через Blutility.
 * UCLASS не оборачивается в #if WITH_EDITOR (UHT должен видеть класс всегда).
 * Реализации методов оборачиваются в #if WITH_EDITOR внутри .cpp.
 */
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
};