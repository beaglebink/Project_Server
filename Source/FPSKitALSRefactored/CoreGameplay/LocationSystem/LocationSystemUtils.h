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

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Address")
    static FLocationAddress BuildAddressForInteriorSet(UInteriorSetAsset* InteriorSet);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Address")
    static FLocationAddress BuildAddressForFloor(UFloorAsset* Floor);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Address")
    static FLocationAddress BuildAddressForZone(
        const FLocationZone& Zone,
        UStreetAsset* OwnerStreet,
        UFloorAsset* OwnerFloor = nullptr);

    // ── Поиск по адресу (по ID) ────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Lookup")
    static UStreetAsset* FindStreetByID(UWorldRegionAsset* Region, const FGuid& StreetID);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Lookup")
    static UInteriorSetAsset* FindInteriorSetByID(UStreetAsset* Street, const FGuid& InteriorSetID);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Lookup")
    static UFloorAsset* FindFloorByID(UInteriorSetAsset* InteriorSet, const FGuid& FloorID);

    // ── Валидация структуры ────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateWorldMap(UWorldMapAsset* WorldMap);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateStreet(UStreetAsset* Street);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateInteriorSet(UInteriorSetAsset* InteriorSet);

    UFUNCTION(BlueprintCallable, Category = "LocationSystem|Validation")
    static FLocationValidationResult ValidateFloor(UFloorAsset* Floor);

#if WITH_EDITOR
    UFUNCTION(CallInEditor, Category="Location|Editor")
    static void OpenFloorAssignerWindow();
#endif

private:
    static void ValidateTransitionPoints(
        const TArray<FLocationTransitionPoint>& Points,
        const FString& OwnerContext,
        FLocationValidationResult& OutResult);
};
