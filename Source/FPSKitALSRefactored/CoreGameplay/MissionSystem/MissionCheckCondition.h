// MissionCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "../InteractionSystem/CheckRequestPayload.h"
#include "MissionCheckCondition.generated.h"

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UMissionCheckCondition : public UCheckCondition
{
    GENERATED_BODY()
public:
    virtual EMissionCheckProperty GetCheckedProperty() const PURE_VIRTUAL(UMissionCheckCondition::GetCheckedProperty, return EMissionCheckProperty::IsActive;);
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const PURE_VIRTUAL(UMissionCheckCondition::CreateRequestPayload, return nullptr;);

    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override;
    virtual void ExecuteCheck(const FGuid& TransactionId) override;
    virtual void OnCheckResponse(const FOutcomeEventBase& Event) override;
    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;

protected:
    FName GetActiveMissionId() const;
};

// 1. Проверка активности (bool)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_IsActive : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    bool ExpectedValue = true;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::IsActive; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// 2. Проверка текущего шага (int)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Step : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    int32 ExpectedValue = 0;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::CurrentStep; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// 3. Проверка названия (string)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Name : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    FString ExpectedValue;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::Name; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};