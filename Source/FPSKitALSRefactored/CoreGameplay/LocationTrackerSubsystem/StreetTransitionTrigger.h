#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StreetTransitionTrigger.generated.h"

class UBoxComponent;
class UStreetAsset;

/**
 * Триггер, публикующий команду «игрок на улице TargetStreet».
 *
 * При входе игрока (любого Pawn) — публикует StreetTransition.
 * При выходе — публикует «покинул улицу» (StreetTransition с bClearStreet=true),
 * но только если за время ClearStreetDelay игрок не вошёл в другой
 * триггер улицы и не оказался внутри дома/этажа.
 *
 * Отложенная очистка защищает от ситуации, когда EndOverlap старого
 * триггера приходит раньше BeginOverlap нового (типичный порядок в UE
 * при переходе через границу двух зон).
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API AStreetTransitionTrigger : public AActor
{
    GENERATED_BODY()

public:
    AStreetTransitionTrigger();

    // Улица, на которую переходит игрок, попадая в этот триггер.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    TObjectPtr<UStreetAsset> TargetStreet;

    // Если true — на выход игрока из триггера публикуется команда
    // «покинул улицу». Если в это же время игрок вошёл в триггер другой
    // улицы, очистка отменяется автоматически.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location")
    bool bClearStreetOnExit = true;

    // Задержка перед публикацией «покинул улицу». Защищает от race
    // с BeginOverlap другого триггера (UE обычно вызывает End(A)
    // до Begin(B) при переходе через границу). 0.05 сек — с запасом.
    // Значение 0 отключает задержку (не рекомендуется).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Location",
        meta = (ClampMin = 0.0, UIMin = 0.0, EditCondition = "bClearStreetOnExit"))
    float ClearStreetDelay = 0.05f;

protected:
    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
    virtual void NotifyActorEndOverlap(AActor* OtherActor) override;

    // Компонент-объём, через который детектится вход/выход Pawn'а.
    // protected: чтобы BP-наследники актора могли с ним работать.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UBoxComponent> TriggerVolume;

private:
    void PublishStreetEnter();
    void PublishStreetClear();
    void ScheduleClearCheck();

    // Таймер отложенной проверки. Один на актор: если игрок вышел — ждём
    // ClearStreetDelay секунд, потом проверяем, что никто не вошёл.
    FTimerHandle ClearCheckTimer;
};