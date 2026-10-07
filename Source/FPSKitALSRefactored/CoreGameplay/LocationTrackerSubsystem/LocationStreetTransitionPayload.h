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
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API ULocationStreetTransitionPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // Целевая улица. Дизайнер ставит soft- или hard-ссылку на UStreetAsset
    // прямо в триггере на уровне.
    UPROPERTY(BlueprintReadWrite, Category = "Location|Street")
    TObjectPtr<UStreetAsset> TargetStreet;

    UFUNCTION(BlueprintCallable, Category = "Location|Street")
    ULocationStreetTransitionPayload* Setup(UStreetAsset* InStreet)
    {
        TargetStreet = InStreet;
        return this;
    }
};