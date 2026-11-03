#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "GameplayTagContainer.h"
#include "BuildingVisitEventPayload.generated.h"

class UInteriorSetAsset;

/**
 * Payload для событий BuildingEntered / BuildingLeft.
 *
 * Публикуется параллельно с общим LocationEntered / LocationLeft
 * (когда Level == Building). Отличие — типизированное поле Building.
 *
 * Спецправило Street «вошёл в дом» не влияет на это событие:
 * BuildingEntered / BuildingLeft отражают именно смену BuildingId
 * в адресе игрока.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UBuildingVisitEventPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Location|Building")
    TObjectPtr<UInteriorSetAsset> Building;

    UPROPERTY(BlueprintReadWrite, Category = "Location|Building")
    FGuid BuildingId;

    UPROPERTY(BlueprintReadWrite, Category = "Location|Building")
    FText DisplayName;

    // Основной тег дома. Копируется из UInteriorSetAsset::LocationTag.
    UPROPERTY(BlueprintReadOnly, Category = "Location|Building|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги категоризации дома.
    UPROPERTY(BlueprintReadOnly, Category = "Location|Building|Tags")
    FGameplayTagContainer ClassificationTags;

    UFUNCTION(BlueprintCallable, Category = "Location|Building")
    UBuildingVisitEventPayload* Setup(
        UInteriorSetAsset* InBuilding,
        const FGuid& InId,
        const FText& InDisplayName,
        const FGameplayTag& InLocationTag,
        const FGameplayTagContainer& InClassificationTags)
    {
        Building = InBuilding;
        BuildingId = InId;
        DisplayName = InDisplayName;
        LocationTag = InLocationTag;
        ClassificationTags = InClassificationTags;
        return this;
    }
};