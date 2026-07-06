// InteriorCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "CheckRequestPayload.h"
#include "InteriorCheckTypes.h"
#include "InteriorCheckCondition.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorCheckCondition : public UCheckCondition
{
    GENERATED_BODY()

public:
    // Тип подсчёта
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    EInteriorActorCountType CountType = EInteriorActorCountType::Registered;

    // Какие типы акторов учитывать (пусто = все)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    TArray<EFloorActorType> FilterTypes;

    // Оператор сравнения
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    // Ожидаемое значение
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior")
    int32 ExpectedValue = 0;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    bool bApproved = false;
    bool bCompleted = false;
};