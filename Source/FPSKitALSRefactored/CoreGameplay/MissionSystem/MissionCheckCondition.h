// MissionCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "CheckRequestPayload.h"
#include "MissionCheckCondition.generated.h"

/**
 * Базовый класс условия проверки для подсистемы миссий.
 * Условие автоматически использует текущую активную миссию.
 * Содержит настройки: какое свойство проверять и ожидаемое значение.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UMissionCheckCondition : public UCheckCondition
{
    GENERATED_BODY()
public:
    // Какое свойство активной миссии проверять (активность, шаг, прогресс, время, статус)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission")
    EMissionCheckProperty PropertyToCheck = EMissionCheckProperty::IsActive;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const PURE_VIRTUAL(UMissionCheckCondition::CreateRequestPayload, return nullptr;);

    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override;
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual void OnCheckResponse(const FOutcomeEventBase& Event) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;

protected:
    // Вспомогательный метод для получения активной миссии (через MissionSubsystem)
    FName GetActiveMissionId() const;
};

// ---- Конкретные проверки для разных типов данных ----

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

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Int : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    int32 ExpectedValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

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