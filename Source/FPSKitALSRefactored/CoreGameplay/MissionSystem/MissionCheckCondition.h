// MissionCheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "CheckCondition.h"
#include "../InteractionSystem/CheckRequestPayload.h"   // <-- исправлен путь
#include "MissionCheckCondition.generated.h"

/**
 * Базовый абстрактный класс условия проверки для подсистемы миссий.
 * Условие автоматически использует текущую активную миссию.
 * Конкретные наследники определяют, какое свойство проверять.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UMissionCheckCondition : public UCheckCondition
{
    GENERATED_BODY()
public:
    // Возвращает свойство, которое проверяет данный класс.
    // Переопределяется в наследниках.
    virtual EMissionCheckProperty GetCheckedProperty() const PURE_VIRTUAL(UMissionCheckCondition::GetCheckedProperty, return EMissionCheckProperty::IsActive;);

    // Создаёт и заполняет Payload запроса (переопределяется в наследниках)
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const PURE_VIRTUAL(UMissionCheckCondition::CreateRequestPayload, return nullptr;);

    // Реализация подписки на ответы – фильтруем Mission::CheckResponse
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const override;

    // Общая реализация ExecuteCheck – получает активную миссию, создаёт Payload, публикует событие, запускает таймер
    virtual void ExecuteCheck(const FGuid& TransactionId) override;

    // Обработка ответа – проверяет TransactionId и подтип, вызывает OnComplete
    virtual void OnCheckResponse(const FOutcomeEventBase& Event) override;

    virtual bool IsApproved() const override { return bApproved; }
    virtual bool IsCompleted() const override { return bCompleted; }
    virtual FString GetDescription() const override;

protected:
    // Вспомогательный метод для получения активной миссии (через MissionSubsystem)
    FName GetActiveMissionId() const;
};

// ===== Конкретные классы для каждого свойства =====

// 1. Проверка активности (bool)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_IsActive : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    bool ExpectedValue = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

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
    int32 ExpectedValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::CurrentStep; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// 3. Проверка прогресса (int)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Progress : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    int32 ExpectedValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::Progress; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// 4. Проверка времени (float)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Time : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    float ExpectedValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::Time; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// 5. Проверка статуса (string)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Status : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    FString ExpectedValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::Status; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};

// 6. Проверка названия миссии (string)
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UMissionCheckCondition_Name : public UMissionCheckCondition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    FString ExpectedValue;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check|Mission|Value")
    ECheckCompareOp Operator = ECheckCompareOp::Equal;

    virtual EMissionCheckProperty GetCheckedProperty() const override { return EMissionCheckProperty::Name; }
    virtual UMissionCheckRequestPayload* CreateRequestPayload() const override;
};