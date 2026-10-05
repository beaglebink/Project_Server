#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "LocationVisitResetPayload.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

/**
 * Payload для команды LocationVisitReset.
 *
 * В Blueprint настраивается каскадно:
 *   1. SelectRegion(Region)              — устанавливает регион
 *   2. GetAvailableStreets()             — возвращает улицы региона
 *   3. SelectStreet(Street)              — устанавливает улицу
 *   4. GetAvailableBuildings()           — возвращает дома улицы
 *   5. SelectBuilding(Building)          — устанавливает дом
 *   6. GetAvailableFloors()              — возвращает этажи дома
 *   7. SelectFloor(Floor)                — устанавливает этаж
 *
 * Трекер сбрасывает визиты для САМОЙ ГЛУБОКОЙ выбранной локации:
 *   Floor    → одна сцена этажа
 *   Building → все этажи дома
 *   Street   → все этажи всех домов улицы
 *   Region   → сцена региона
 *   Map      → сцены всех регионов карты
 *   ничего + bResetAll → вся история
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API ULocationVisitResetPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // Сбрасывает всю историю посещений. Иерархия игнорируется.
    UPROPERTY(BlueprintReadWrite, Category = "LocationTracker")
    bool bResetAll = false;

    // ── Иерархия цели ────────────────────────────────────────────────────
    UPROPERTY(BlueprintReadWrite, Category = "LocationTracker|Target")
    TObjectPtr<UWorldMapAsset> TargetMap;

    UPROPERTY(BlueprintReadWrite, Category = "LocationTracker|Target")
    TObjectPtr<UWorldRegionAsset> TargetRegion;

    UPROPERTY(BlueprintReadWrite, Category = "LocationTracker|Target")
    TObjectPtr<UStreetAsset> TargetStreet;

    UPROPERTY(BlueprintReadWrite, Category = "LocationTracker|Target")
    TObjectPtr<UInteriorSetAsset> TargetBuilding;

    UPROPERTY(BlueprintReadWrite, Category = "LocationTracker|Target")
    TObjectPtr<UFloorAsset> TargetFloor;

    // ── Каскадная настройка ─────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SetupAll();

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SelectMap(UWorldMapAsset* InMap);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SelectRegion(UWorldRegionAsset* InRegion);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SelectStreet(UStreetAsset* InStreet);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SelectBuilding(UInteriorSetAsset* InBuilding);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SelectFloor(UFloorAsset* InFloor);

    // ── Списки для каскадных комбо-боксов ───────────────────────────────

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UWorldRegionAsset*> GetAvailableRegions() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UStreetAsset*> GetAvailableStreets() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UInteriorSetAsset*> GetAvailableBuildings() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UFloorAsset*> GetAvailableFloors() const;

    // Одностадийный Setup — вся цепочка сразу, без каскада.
    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* Setup(
        UWorldMapAsset* InMap,
        UWorldRegionAsset* InRegion,
        UStreetAsset* InStreet,
        UInteriorSetAsset* InBuilding,
        UFloorAsset* InFloor);
};