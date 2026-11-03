#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "GameplayTagContainer.h"
#include "StreetVisitEventPayload.generated.h"

class UStreetAsset;

/**
 * Payload для событий StreetEntered / StreetLeft.
 *
 * Публикуется параллельно с общим LocationEntered / LocationLeft
 * (когда Level == Street). Отличие — типизированное поле Street,
 * без необходимости Cast в Blueprint.
 *
 * Спецправило Street: «вошёл в дом на улице S» тоже даёт StreetLeft(S),
 * а «вышел из дома на улицу S» — StreetEntered(S). То есть с точки зрения
 * этого события игрок «покинул улицу» в момент входа в дом на ней,
 * и «вернулся на улицу» в момент выхода. Это согласовано с
 * ULocationTrackerSubsystem::HasLeftAndReturned.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UStreetVisitEventPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Location|Street")
    TObjectPtr<UStreetAsset> Street;

    UPROPERTY(BlueprintReadWrite, Category = "Location|Street")
    FGuid StreetId;

    UPROPERTY(BlueprintReadWrite, Category = "Location|Street")
    FText DisplayName;

    // Основной тег улицы. Копируется из UStreetAsset::LocationTag.
    UPROPERTY(BlueprintReadOnly, Category = "Location|Street|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги категоризации улицы.
    UPROPERTY(BlueprintReadOnly, Category = "Location|Street|Tags")
    FGameplayTagContainer ClassificationTags;

    UFUNCTION(BlueprintCallable, Category = "Location|Street")
    UStreetVisitEventPayload* Setup(
        UStreetAsset* InStreet,
        const FGuid& InId,
        const FText& InDisplayName,
        const FGameplayTag& InLocationTag,
        const FGameplayTagContainer& InClassificationTags)
    {
        Street = InStreet;
        StreetId = InId;
        DisplayName = InDisplayName;
        LocationTag = InLocationTag;
        ClassificationTags = InClassificationTags;
        return this;
    }
};