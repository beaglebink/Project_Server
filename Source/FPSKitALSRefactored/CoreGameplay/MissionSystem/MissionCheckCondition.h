// MissionCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "CheckRequestPayload.h"
#include "MissionCheckCondition.generated.h"

/**
 * Базовый класс условия проверки для подсистемы миссий.
 * Содержит идентификатор миссии и виртуальный метод для создания запроса.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UMissionCheckCondition : public UCheckCondition
{
    GENERATED_BODY()
public:
    // Идентификатор миссии, которую проверяем
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission")
    FName MissionId;

    // Какое свойство миссии проверять (активность, шаг, прогресс, время, статус)
    // Теперь в основной категории, чтобы отображаться перед Value
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission")
    EMissionCheckProperty PropertyToCheck = EMissionCheckProperty::IsActive;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const PURE_VIRTUAL(UMissionCheckCondition::CreateRequestPayload, return nullptr;);

    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override;
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual void OnCheckResponse(const FOutcomeEventBase& Event) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;
};

// ---- Конкретные проверки для разных типов данных ----

// Проверка bool
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Bool : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    bool ExpectedValue = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// Проверка int32
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Int : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    int32 ExpectedValue = 0;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// Проверка float
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Float : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    float ExpectedValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// Проверка FString
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_String : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    FString ExpectedValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};