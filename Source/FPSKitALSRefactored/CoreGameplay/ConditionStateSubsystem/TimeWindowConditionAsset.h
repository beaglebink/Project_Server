#pragma once

#include "CoreMinimal.h"
#include "OutcomeConditionAsset.h"
#include "TimeWindowConditionAsset.generated.h"

UENUM(BlueprintType)
enum class ETimeWindowMode : uint8
{
    // Истинно, пока с момента подъёма прошло < WindowSeconds.
    // Пример: «доступно в течение 60 секунд после события».
    WithinWindow UMETA(DisplayName = "Within window (available for X sec)"),

    // Истинно, когда с момента подъёма прошло >= WindowSeconds.
    // Пример: «доступно через 60 секунд после события».
    AfterWindow  UMETA(DisplayName = "After window (available after X sec)"),
};

/**
 * Обёртка над любым UOutcomeConditionAsset.
 *
 * Позволяет применять к результату внутреннего условия временное окно,
 * опираясь на момент, когда внутреннее условие стало истинным.
 *
 * Когда ChildCondition становится истинным — обёртка запоминает этот момент
 * (в игровом времени, через UConditionStateSubsystem). Пока ChildCondition
 * остаётся истинным, окно продолжает считаться от зафиксированного момента.
 * Как только ChildCondition становится ложным — метка сбрасывается, и
 * следующее срабатывание начнёт новое окно.
 *
 * Особенности:
 *   • Работает с любым типом условия: Location, Chore, WorldState,
 *     Mission, Terminal, композитами.
 *   • Не требует модификации оборачиваемого условия.
 *   • Переживает save/load (состояние — в UConditionStateSubsystem).
 *   • Момент подъёма фиксируется с точностью до частоты переоценки
 *     (в хорах — раз в AvailabilityPulseIntervalSeconds секунд, плюс
 *     при каждом событии через EventBus).
 */
UCLASS(BlueprintType, ShowCategories = ("Time Window", "4 - Debug"))
class FPSKITALSREFACTORED_API UTimeWindowConditionAsset : public UOutcomeConditionAsset
{
    GENERATED_BODY()

public:
    // Условие, над которым надстраивается окно.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Window")
    TObjectPtr<UOutcomeConditionAsset> ChildCondition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Window")
    ETimeWindowMode TimeMode = ETimeWindowMode::WithinWindow;

    // Длительность окна в секундах.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Window",
        meta = (ClampMin = 0.0f))
    float WindowSeconds = 60.0f;

    // Явный ключ для хранения метки в подсистеме. Если NAME_None —
    // используется путь ассета. Указывайте явно, только если у вас
    // несколько обёрток, которые должны разделять общее окно.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Time Window")
    FName StateKeyOverride;

    virtual bool IsStateDriven() const override
    {
        return ChildCondition && ChildCondition->IsStateDriven();
    }

    virtual void CompileCondition() override;
    bool EvaluateCondition(const FOutcomeEventBase& Outcome) const;
};