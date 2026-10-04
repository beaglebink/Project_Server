#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "CheckRequestPayload.h"
#include "LocationConditionAsset.generated.h"

class UWorldMapAsset;
class UWorldRegionAsset;
class UStreetAsset;
class UInteriorSetAsset;
class UFloorAsset;

UENUM(BlueprintType)
enum class ELocationQueryType : uint8
{
    LeftAndReturned     UMETA(DisplayName = "Left And Returned"),
    HasEnteredEver      UMETA(DisplayName = "Has Entered Ever"),
    VisitCountAtLeast   UMETA(DisplayName = "Visit Count At Least")
};

UCLASS(BlueprintType, ShowCategories = ("Location", "4 - Debug"))
class FPSKITALSREFACTORED_API ULocationConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    ELocationQueryType QueryType = ELocationQueryType::LeftAndReturned;

    // ── Иерархия цели ────────────────────────────────────────────────────
    // Заполняйте до нужной глубины. Используется САМОЕ ГЛУБОКОЕ указанное поле:
    //   Floor   → одна сцена этажа.
    //   Building (без Floor) → «любой из этажей этого здания».
    //   Street   (без Building) → «любой этаж любого здания этой улицы».
    //   Region   (без Street) → сцена региона.
    //   Map      (без Region) → любая сцена региона из этой карты.
    // Более общие поля, если они заданы вместе с более глубокими,
    // используются только для проверки консистентности цепочки.

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Target")
    TObjectPtr<UWorldMapAsset> TargetMap;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Target")
    TObjectPtr<UWorldRegionAsset> TargetRegion;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Target")
    TObjectPtr<UStreetAsset> TargetStreet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Target")
    TObjectPtr<UInteriorSetAsset> TargetBuilding;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location|Target")
    TObjectPtr<UFloorAsset> TargetFloor;

    // Порог для VisitCountAtLeast (сумма EnterCount по всем целевым сценам).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition = "QueryType == ELocationQueryType::VisitCountAtLeast",
            EditConditionHides, ClampMin = 1))
    int32 VisitThreshold = 1;

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    // Собирает нормализованные имена пакетов сцен, соответствующих цели.
    bool ResolveTargetPackageNames(TArray<FString>& OutPackageNames) const;

    // Проверка консистентности цепочки (Floor ⊆ Building ⊆ Street ⊆ Region ⊆ Map).
    bool ValidateHierarchy(FString& OutError) const;

    static class ULocationTrackerSubsystem* FindTracker();
};