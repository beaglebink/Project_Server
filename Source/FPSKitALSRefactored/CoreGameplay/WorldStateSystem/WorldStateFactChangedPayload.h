#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "WorldStateTypes.h"
#include "WorldStateFactChangedPayload.generated.h"

/**
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
    // Идентификатор факта (дублируется из Record для удобства BP-графов).
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FName FactId;

    // Снимок записи на момент события.
    //   Added   → новая запись (bPendingRemoval = false)
    //   Changed → запись с новым SerializedValue
    //   Removed → запись до пометки на удаление (bPendingRemoval = false)
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FWorldStateRecord Record;

    // Только для Changed: предыдущее значение SerializedValue.
    // Для Added и Removed — пустая строка.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString PreviousValue;

    // Только для Removed: значение, которое будет восстановлено на актёре
    // (OriginalValue записи). Значимо только при bHasRestoredValue = true.
    UPROPERTY(BlueprintReadWrite, Category = "WorldState")
    FString RestoredValue;

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