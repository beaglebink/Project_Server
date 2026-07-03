// CheckCondition.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EventBusSubsystem.h"
#include "CheckRequestPayload.h"
#include "CheckResponsePayload.h"
#include "OutcomeConditionAsset.h"
#include "CheckCondition.generated.h"

class UCheckCoordinatorComponent;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class FPSKITALSREFACTORED_API UCheckCondition : public UObject
{
    GENERATED_BODY()
public:
    // Инициализация – здесь подписываемся на ответы
    virtual void Initialize(UCheckCoordinatorComponent* InCoordinator, UEventBusSubsystem* InEventBus);

    // Запуск проверки – отправляет запрос и запускает таймаут
    virtual void ExecuteCheck(const FGuid& TransactionId) PURE_VIRTUAL(UCheckCondition::ExecuteCheck, );

    // Обработчик ответа (вызывается EventBus'ом)
    virtual void OnCheckResponse(const FOutcomeEventBase& Event) PURE_VIRTUAL(UCheckCondition::OnCheckResponse, );

    // Результаты
    virtual bool IsApproved() const PURE_VIRTUAL(UCheckCondition::IsApproved, return false;);
    virtual bool IsCompleted() const PURE_VIRTUAL(UCheckCondition::IsCompleted, return false;);

    // Описание для отладки
    virtual FString GetDescription() const PURE_VIRTUAL(UCheckCondition::GetDescription, return TEXT(""); );

    // Уведомление координатора о завершении
    DECLARE_DELEGATE_OneParam(FOnConditionComplete, UCheckCondition*);
    FOnConditionComplete OnComplete;

    // Возвращает TransactionId текущей проверки
    FGuid GetCurrentTransactionId() const { return CurrentTransactionId; }

    // Таймаут в секундах (можно настраивать в редакторе)
    //UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Check")
    float TimeoutSeconds = 3.0f;

protected:
    UPROPERTY()
    TObjectPtr<UCheckCoordinatorComponent> Coordinator;

    UPROPERTY()
    TObjectPtr<UEventBusSubsystem> EventBus;

    FGuid CurrentTransactionId;

    // Создаёт ConditionAsset для фильтрации ответов (переопределяется в наследниках)
    virtual UOutcomeConditionAsset* CreateSubscriptionCondition() const PURE_VIRTUAL(UCheckCondition::CreateSubscriptionCondition, return nullptr;);

    // Запуск таймаута
    void StartTimeoutTimer(const FGuid& TransactionId);

    // Остановка таймаута
    void ClearTimeoutTimer();

    // Обработчик таймаута
    void OnTimeout(FGuid TransactionId);

    UWorld* GetWorld() const;

    // Флаги состояния
    bool bCompleted = false;
    bool bApproved = false;

private:
    FTimerHandle TimeoutTimerHandle;
};