#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "CheckRequestPayload.h"
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

    // Имя локации, как его видит дизайнер в редакторе. Совпадает с DisplayName
    // одного из ассетов иерархии локаций:
    //   Floor (UFloorAsset) / Building (UInteriorSetAsset) /
    //   Street (UStreetAsset) / Region (UWorldRegionAsset) / Map (UWorldMapAsset).
    // Если совпадений несколько — используются все.
    // Резолв делается один раз при CompileCondition и кэшируется.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    FText TargetDisplayName;

    // Операция сравнения для VisitCountAtLeast.
    // Применяется к сумме EnterCount по всем целевым сценам.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition = "QueryType == ELocationQueryType::VisitCountAtLeast",
            EditConditionHides))
    ECheckCompareOp CompareOp = ECheckCompareOp::GreaterOrEqual;

    // Порог для VisitCountAtLeast (сумма EnterCount по всем целевым сценам).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (EditCondition = "QueryType == ELocationQueryType::VisitCountAtLeast",
            EditConditionHides, ClampMin = 1))
    int32 VisitThreshold = 1;

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;

private:
    // Резолвит TargetDisplayName в список нормализованных имён пакетов сцен.
    // Заполняет CachedTargetPackageNames. Вызывается из CompileCondition.
    void ResolveTargets();

    static class ULocationTrackerSubsystem* FindTracker();

    // Кэш резолва. mutable — потому что EvaluateCondition const, но нам нужно
    // переиспользовать результат CompileCondition.
    TArray<FString> CachedTargetPackageNames;
};