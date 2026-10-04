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
    CurrentlyAtTarget   UMETA(DisplayName = "Currently At Target (state)"),
    HasEnteredEver      UMETA(DisplayName = "Has Entered Ever (state)"),
    HasEnteredRecently  UMETA(DisplayName = "Has Entered Recently (state)"),
    LeftAndReturned     UMETA(DisplayName = "Left And Returned (state)"),
    VisitCountAtLeast   UMETA(DisplayName = "Visit Count At Least (state)")
};

UCLASS(BlueprintType, ShowCategories = ("Location", "4 - Debug"))
class FPSKITALSREFACTORED_API ULocationConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    ELocationQueryType QueryType = ELocationQueryType::CurrentlyAtTarget;

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

    // ── Параметры ────────────────────────────────────────────────────────
    // Окно в минутах для HasEnteredRecently / LeftAndReturned. 0 = без ограничения.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition =
            "QueryType == ELocationQueryType::HasEnteredRecently || QueryType == ELocationQueryType::LeftAndReturned",
            EditConditionHides, ClampMin = 0.0f))
    float TimeWindowMinutes = 0.0f;

    // Порог для VisitCountAtLeast (сумма EnterCount по всем целевым сценам).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition = "QueryType == ELocationQueryType::VisitCountAtLeast",
            EditConditionHides, ClampMin = 1))
    int32 VisitThreshold = 1;

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    // Собирает нормализованные имена пакетов сцен, соответствующих цели.
    // Возвращает false, если цель не задана или цепочка иерархии несогласована.
    bool ResolveTargetPackageNames(TArray<FString>& OutPackageNames) const;

    // Проверка консистентности цепочки (Floor ⊆ Building ⊆ Street ⊆ Region ⊆ Map).
    // Возвращает true, если всё согласовано; иначе в OutError пишет текст проблемы.
    bool ValidateHierarchy(FString& OutError) const;

    // Найти трекер (как и раньше — через контексты мира).
    static class ULocationTrackerSubsystem* FindTracker();
};