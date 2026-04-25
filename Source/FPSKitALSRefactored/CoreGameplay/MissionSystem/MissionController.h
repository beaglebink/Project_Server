#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MissionAsset.h"
#include "MissionEnvelopeTypes.h"
#include "OutcomeEventBase.h"
#include "MissionController.generated.h"

class UMissionSubsystem;
class UEventBusSubsystem;

// ??? EMissionStatus ???????????????????????????????????????????????????????????
// Текущий статус экземпляра миссии.
UENUM(BlueprintType)
enum class EMissionStatus : uint8
{
    // Создан, но ещё не активирован
    Inactive    UMETA(DisplayName = "Inactive"),
    // Активен — идёт выполнение
    Active      UMETA(DisplayName = "Active"),
    // Приостановлен (dormant), например при выходе из здания
    Suspended   UMETA(DisplayName = "Suspended"),
    // Разрешён: цели выполнены / провалены / заброшены; envelope ещё может владеть состоянием
    Resolved    UMETA(DisplayName = "Resolved"),
    // Завершён: envelope освобождён, состояние применено или сброшено
    Released    UMETA(DisplayName = "Released")
};

// ??? UMissionController ???????????????????????????????????????????????????????
// Базовый C++ класс для Blueprint-логики миссии.
// Жизненный цикл: создаётся через MissionSubsystem::CreateMission(),
//                 уничтожается при переходе в Released.
//
// В Blueprint-наследнике:
//   - переопределить OnMissionActivated  — стартовая логика
//   - переопределить OnMissionResolved   — реакция на завершение
//   - переопределить OnBuildingExited    — реакция на выход из здания
//   - вызывать RequestResolve() когда цели выполнены / провалены / заброшены
UCLASS(Blueprintable, BlueprintType, Category = "Mission")
class FPSKITALSREFACTORED_API UMissionController : public UObject
{
    GENERATED_BODY()

public:
    // ?? Инициализация ?????????????????????????????????????????????????????????

    // Вызывается MissionSubsystem после создания. Не вызывать напрямую из Blueprint.
    void InitFromAsset(UMissionAsset* InAsset, UGameInstance* InOwner);

    // ?? Публичный API ?????????????????????????????????????????????????????????

    // Активировать миссию (вызывает OnMissionActivated)
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void Activate();

    // Запросить разрешение с указанием причины завершения
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void RequestResolve(EMissionEndReason Reason);

    // Приостановить (dormant). Вызывается при выходе из здания если envelope не освобождается.
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void Suspend();

    // Возобновить после приостановки
    UFUNCTION(BlueprintCallable, Category = "Mission|Control")
    void Resume();

    // ?? Геттеры ???????????????????????????????????????????????????????????????

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Info")
    FName GetMissionId() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Info")
    EMissionStatus GetStatus() const { return Status; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Info")
    EMissionEndReason GetEndReason() const { return EndReason; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Info")
    UMissionAsset* GetMissionAsset() const { return MissionAsset; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Info")
    const FMissionEnvelope& GetEnvelope() const;

    // true если envelope имеет смысл (не пустой scope)
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Mission|Info")
    bool HasValidEnvelope() const;

    // ?? Уведомления от MissionSubsystem ??????????????????????????????????????

    // Вызывается когда игрок покидает здание, входящее в Scope
    UFUNCTION(BlueprintCallable, Category = "Mission|Events")
    void NotifyBuildingExited();

    // ?? Blueprint-события (переопределяются в наследнике) ?????????????????????

    // Вызывается при активации миссии
    UFUNCTION(BlueprintNativeEvent, Category = "Mission|Events")
    void OnMissionActivated();
    virtual void OnMissionActivated_Implementation() {}

    // Вызывается при разрешении (Completed / Failed / Abandoned)
    UFUNCTION(BlueprintNativeEvent, Category = "Mission|Events")
    void OnMissionResolved(EMissionEndReason Reason);
    virtual void OnMissionResolved_Implementation(EMissionEndReason Reason) {}

    // Вызывается при выходе из здания (до принятия решения об Envelope)
    UFUNCTION(BlueprintNativeEvent, Category = "Mission|Events")
    void OnBuildingExited();
    virtual void OnBuildingExited_Implementation() {}

    // Вызывается при приостановке
    UFUNCTION(BlueprintNativeEvent, Category = "Mission|Events")
    void OnMissionSuspended();
    virtual void OnMissionSuspended_Implementation() {}

    // Вызывается при возобновлении
    UFUNCTION(BlueprintNativeEvent, Category = "Mission|Events")
    void OnMissionResumed();
    virtual void OnMissionResumed_Implementation() {}

    // Необходимо для работы NewObject с GameInstance как Outer
    virtual UWorld* GetWorld() const override;

private:
    UPROPERTY()
    TObjectPtr<UMissionAsset> MissionAsset;

    UPROPERTY()
    TWeakObjectPtr<UGameInstance> OwnerGameInstance;

    EMissionStatus Status = EMissionStatus::Inactive;
    EMissionEndReason EndReason = EMissionEndReason::None;

    // Публикует событие на EventBus об изменении статуса миссии
    void BroadcastStatusChanged();
};
