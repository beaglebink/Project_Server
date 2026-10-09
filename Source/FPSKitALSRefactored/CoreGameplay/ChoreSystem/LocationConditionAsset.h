#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "CheckRequestPayload.h"
#include "../LocationTrackerSubsystem/LocationTrackerSubsystem.h"   // ELocationLevel, FLocationVisitKey
#include "LocationConditionAsset.generated.h"

UENUM(BlueprintType)
enum class ELocationQueryType : uint8
{
    HasEnteredEver      UMETA(DisplayName = "Has Entered Ever (state)"),
    LeftAndReturned     UMETA(DisplayName = "Left And Returned (state)"),
    VisitCountAtLeast   UMETA(DisplayName = "Visit Count At Least (state)")
};

UCLASS(BlueprintType, ShowCategories = ("Location", "4 - Debug"))
class FPSKITALSREFACTORED_API ULocationConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    ELocationQueryType QueryType = ELocationQueryType::HasEnteredEver;

    // Отображаемое имя локации, как его видит дизайнер в редакторе.
    // Совпадает с DisplayName одного из ассетов: Floor / InteriorSet / Street /
    // WorldRegion / WorldMap. Резолвится один раз в CompileCondition.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    FText TargetDisplayName;

    // Ограничение по уровню иерархии при резолве TargetDisplayName.
    //
    // Default  — прежнее поведение: обход всех типов, приоритет
    //            Floor → Building → Street → Region → Map. Подходит,
    //            когда DisplayName уникален в проекте.
    // Floor / Building / Street / Region / Map — искать только среди ассетов
    //            указанного уровня. Нужно, когда DisplayName может совпасть
    //            у разных типов (например, улица и одноимённый дом на ней).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    ELocationLevel PreferredLevel = ELocationLevel::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition = "QueryType == ELocationQueryType::VisitCountAtLeast",
            EditConditionHides))
    ECheckCompareOp CompareOp = ECheckCompareOp::GreaterOrEqual;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition = "QueryType == ELocationQueryType::VisitCountAtLeast",
            EditConditionHides, ClampMin = 0))
    int32 VisitThreshold = 1;

    virtual bool IsStateDriven() const override { return true; }

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    static class ULocationTrackerSubsystem* FindTracker();

    // Кэш резолва TargetDisplayName.
    FLocationVisitKey CachedKey;
};