#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "LocationStreetTransitionPayload.generated.h"

class UStreetAsset;

/**
 * Payload для команды «игрок перешёл на улицу».
 *
 * Публикуется триггерами, лежащими на улицах, когда игрок входит в их зону.
 * Трекер локаций обновляет CurrentAddress: улица (и, при необходимости,
 * регион/карта из её иерархии) перезаписываются значениями из TargetStreet,
 * а BuildingId/FloorId сбрасываются — игрок считается стоящим на улице.
 *
 * Идемпотентность: если игрок уже стоит на этой улице (та же улица,
 * регион и карта, Building/Floor пусты) — команда игнорируется.
 *
 * Если игрок был в доме на этой же улице и приходит команда «на улицу» —
 * это корректный выход из дома: адрес обновляется, а спецправило
 * Street в ApplyAddressTransition добавит Enter[Street].
 *
 * Сценарий очистки (bClearStreet = true):
 * Игрок вышел из всех триггеров улицы и не вошёл в триггер другой.
 * Трекер убирает StreetId из CurrentAddress, но сохраняет RegionId/MapId —
 * то есть «игрок в регионе, но не на конкретной улице». Это нужно,
 * чтобы HasEnteredEver / HasLeftAndReturned для улицы корректно
 * фиксировали уход, а регион оставался актуальным.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API ULocationStreetTransitionPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // Целевая улица. Дизайнер ставит soft- или hard-ссылку на UStreetAsset
    // прямо в триггере на уровне.
    // В режиме bClearStreet=true используется ТОЛЬКО для получения
    // родительского региона/карты — StreetId в новый адрес не попадёт.
    UPROPERTY(BlueprintReadWrite, Category = "Location|Street")
    TObjectPtr<UStreetAsset> TargetStreet;

    // Если true — команда означает «игрок покинул улицу». Трекер сохраняет
    // RegionId/MapId, но обнуляет StreetId. Полезно для выхода из зоны
    // улицы, когда игрок не вошёл в зону другой.
    UPROPERTY(BlueprintReadWrite, Category = "Location|Street")
    bool bClearStreet = false;

    UFUNCTION(BlueprintCallable, Category = "Location|Street")
    ULocationStreetTransitionPayload* Setup(UStreetAsset* InStreet, bool bInClearStreet = false)
    {
        TargetStreet = InStreet;
        bClearStreet = bInClearStreet;
        return this;
    }
};