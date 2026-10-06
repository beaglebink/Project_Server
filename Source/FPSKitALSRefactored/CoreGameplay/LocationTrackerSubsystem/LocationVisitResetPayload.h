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
 * Два способа настройки:
 *
 *  1) Каскадный (для UI с ComboBox'ами):
 *     SelectRegion / SelectStreet / SelectBuilding / SelectFloor — устанавливают
 *     выбранный ассет и подтягивают/сбрасывают соседние уровни.
 *     GetAvailable* — возвращают список ассетов для текущего контекста.
 *
 *  2) Прямой по DisplayName (для скриптов и настроек, где ассеты не под рукой):
 *     SetupRegion / SetupStreet / SetupBuilding / SetupFloor — принимают FText,
 *     совпадающий с DisplayName нужного ассета, и сами находят его в иерархии.
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

    // ── Сбросить всё ────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup")
    ULocationVisitResetPayload* SetupAll();

    // ── Каскадная настройка (для ComboBox'ов) ────────────────────────────

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

    // ── Списки для каскадных ComboBox'ов ────────────────────────────────

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UWorldRegionAsset*> GetAvailableRegions() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UStreetAsset*> GetAvailableStreets() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UInteriorSetAsset*> GetAvailableBuildings() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "LocationTracker|Options")
    TArray<UFloorAsset*> GetAvailableFloors() const;

    // ── Прямая настройка по DisplayName ─────────────────────────────────
    // Каждая функция:
    //   1) Ищет ассет по DisplayName. Если у payload уже задан родитель
    //      нужного уровня — ищет строго внутри него; иначе — по всем ассетам.
    //   2) Устанавливает найденный ассет в соответствующее поле.
    //   3) Подтягивает всю родительскую цепочку из soft-ссылок ассета.
    //   4) Обнуляет все более глубокие уровни.
    //
    // Если ассет с таким DisplayName не найден — пишет Warning,
    // payload не меняет.

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup|ByName")
    ULocationVisitResetPayload* SetupMap(const FText InDisplayName);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup|ByName")
    ULocationVisitResetPayload* SetupRegion(const FText InDisplayName);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup|ByName")
    ULocationVisitResetPayload* SetupStreet(const FText InDisplayName);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup|ByName")
    ULocationVisitResetPayload* SetupBuilding(const FText InDisplayName);

    UFUNCTION(BlueprintCallable, Category = "LocationTracker|Setup|ByName")
    ULocationVisitResetPayload* SetupFloor(const FText InDisplayName);
};