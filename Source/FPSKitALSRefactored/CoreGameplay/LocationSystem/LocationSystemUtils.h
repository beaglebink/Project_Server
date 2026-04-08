#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LocationTypes.h"
#include "LocationSpatialTypes.h"
#include "LocationSystemUtils.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

// Результат проверки "висячих" ссылок
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Validation")
    bool bIsValid = true;

    // Список описаний проблем (для лога и редактора)
    UPROPERTY(BlueprintReadOnly, Category = "Validation")
    TArray<FString> Issues;

    void AddIssue(const FString& Issue)
    {
        bIsValid = false;
        Issues.Add(Issue);
    }
};

UCLASS()
class FPSKITALSREFACTORED_API ULocationSystemUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ── Построение читаемого адреса ────────────────────────────────────────

    // Строит адрес для InteriorSet (до здания включительно)
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Address")
    static FLocationAddress BuildAddressForInteriorSet(UInteriorSetAsset* InteriorSet);

    // Строит адрес для Floor внутри здания
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Address")
    static FLocationAddress BuildAddressForFloor(UFloorAsset* Floor);

    // Строит адрес для Zone (на улице или на этаже)
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Address")
    static FLocationAddress BuildAddressForZone(
        const FLocationZone& Zone,
        UStreetAsset* OwnerStreet,
        UFloorAsset* OwnerFloor = nullptr);

    // ── Поиск по адресу (по ID) ────────────────────────────────────────────

    // Ищет Street по ID в заданном регионе
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Lookup")
    static UStreetAsset* FindStreetByID(UWorldRegionAsset* Region, const FGuid& StreetID);

    // Ищет InteriorSet по ID на заданной улице
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Lookup")
    static UInteriorSetAsset* FindInteriorSetByID(UStreetAsset* Street, const FGuid& InteriorSetID);

    // Ищет Floor по ID в InteriorSet
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Lookup")
    static UFloorAsset* FindFloorByID(UInteriorSetAsset* InteriorSet, const FGuid& FloorID);

    // ── Валидация структуры ────────────────────────────────────────────────

    // Проверяет всю иерархию начиная с WorldMap (выявляет висячие ссылки)
    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateWorldMap(UWorldMapAsset* WorldMap);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateStreet(UStreetAsset* Street);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateInteriorSet(UInteriorSetAsset* InteriorSet);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateFloor(UFloorAsset* Floor);

private:
    // Проверка TransitionPoint: обе стороны должны быть валидными GUID
    static void ValidateTransitionPoints(
        const TArray<FLocationTransitionPoint>& Points,
        const FString& OwnerContext,
        FLocationValidationResult& OutResult);
};