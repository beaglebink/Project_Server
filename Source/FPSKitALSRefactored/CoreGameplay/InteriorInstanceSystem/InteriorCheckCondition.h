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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior")
    EInteriorActorCountType CountType = EInteriorActorCountType::Registered;

    // Какие типы акторов учитывать (пусто = все)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior")
    TArray<EFloorActorType> FilterTypes;

    // Ожидаемое значение
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior")
    int32 ExpectedValue = 0;

    // Оператор сравнения
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Interior")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override { return nullptr; }

protected:
    bool bApproved = false;
    bool bCompleted = false;
};