#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "WorldStateTypes.h"
#include "WorldStateFactChangedPayload.generated.h"

/**
 * Payload for WorldStateFactAdded / WorldStateFactChanged / WorldStateFactRemoved events.
 *
 * Contains a full snapshot of the record at the time of the event — the subscriber does not need
 * to re-query the subsystem to find out the state of the fact.
 *
 * Payload для событий WorldStateFactAdded / WorldStateFactChanged / WorldStateFactRemoved.
 *
 * Содержит полный снимок записи на момент события — подписчику не нужно
 * повторно обращаться к подсистеме, чтобы узнать состояние факта.
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UWorldStateFactChangedPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    // Fact identifier (duplicated from Record for convenience of BP graphs).
    // Идентификатор факта (дублируется из Record для удобства BP-графов).
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName FactId;

    // Snapshot of the record at the time of the event.
    //   Added   → new record (bPendingRemoval = false)
    //   Changed → record with the new SerializedValue
    //   Removed → record before being marked for removal (bPendingRemoval = false)
    // Снимок записи на момент события.
    //   Added   → новая запись (bPendingRemoval = false)
    //   Changed → запись с новым SerializedValue
    //   Removed → запись до пометки на удаление (bPendingRemoval = false)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FWorldStateRecord Record;

    // Only for Changed: previous value of SerializedValue.
    // For Added and Removed — empty string.
    // Только для Changed: предыдущее значение SerializedValue.
    // Для Added и Removed — пустая строка.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString PreviousValue;

    // Only for Removed: the value that will be restored on the actor
    // (OriginalValue of the record). Only meaningful when bHasRestoredValue = true.
    // Только для Removed: значение, которое будет восстановлено на актёре
    // (OriginalValue записи). Значимо только при bHasRestoredValue = true.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString RestoredValue;

    // true if the value to restore is known for Removed.
    // false → the original was not captured (the actor never appeared),
    //         there is nothing to restore.
    // true, если для Removed известно восстанавливаемое значение.
    // false → оригинал не был захвачен (актёр никогда не появлялся),
    //         восстанавливать нечего.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    bool bHasRestoredValue = false;

    UFUNCTION(BlueprintCallable, Category = "WorldState")
    UWorldStateFactChangedPayload* Setup(FName InFactId, const FWorldStateRecord& InRecord)
    {
        FactId = InFactId;
        Record = InRecord;
        return this;
    }
};