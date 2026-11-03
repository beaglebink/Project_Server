#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "GameplayTagContainer.h"
#include "LocationTypes.h"
#include "LocationVisitEventPayload.generated.h"

/**
 * Payload для событий «игрок вошёл в локацию» / «игрок покинул локацию».
 *
 * Публикуется LocationTrackerSubsystem в ApplyAddressTransition в момент,
 * когда меняется счётчик посещений:
 *   - LocationEntered — EnterCount[Level][LocationId]++
 *   - LocationLeft    — LeaveCount[Level][LocationId]++
 *
 * ВАЖНО про Street и спецправила:
 * Трекер считает «игрок вышел из открытой улицы» в момент входа в дом
 * на этой улице (Leave[Street]++) и «вернулся на улицу» в момент выхода
 * из дома (Enter[Street]++). Соответственно, при заходе в дом вы получите
 * события:
 *   LocationLeft(Street=..., Building=...)
 *   LocationEntered(Building=..., Floor=...)
 * — то есть Street «покинут», даже если физически вы ещё на территории улицы.
 * Это осознанное поведение, согласованное с HasLeftAndReturned.
 *
 * Если подписчику нужно «где игрок сейчас» — используйте
 * ULocationTrackerSubsystem::GetCurrentLocationAsset / GetCurrentDeepestLevel /
 * GetCurrentLocationDebugString, а не сам payload.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API ULocationVisitEventPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "Location")
    ELocationLevel Level = ELocationLevel::Default;

    UPROPERTY(BlueprintReadWrite, Category = "Location")
    FGuid LocationId;

    // Ассет соответствующего уровня:
    //   Map      → UWorldMapAsset
    //   Region   → UWorldRegionAsset
    //   Street   → UStreetAsset
    //   Building → UInteriorSetAsset
    //   Floor    → UFloorAsset
    // Может быть nullptr, если GUID не привязан к известному ассету —
    // тогда ориентируйтесь на LocationId.
    UPROPERTY(BlueprintReadWrite, Category = "Location")
    TObjectPtr<UObject> LocationAsset;

    UPROPERTY(BlueprintReadWrite, Category = "Location")
    FText DisplayName;

    // Основной тег локации. Копируется из LocationTag ассета при публикации.
    // Для Map/Region/Street/Building/Floor — соответствующий ассет.
    // Может быть невалидным, если у ассета поле не заполнено.
    UPROPERTY(BlueprintReadOnly, Category = "Location|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги категоризации, копируются из ClassificationTags
    // ассета при публикации.
    UPROPERTY(BlueprintReadOnly, Category = "Location|Tags")
    FGameplayTagContainer ClassificationTags;

    UFUNCTION(BlueprintCallable, Category = "Location")
    ULocationVisitEventPayload* Setup(
        ELocationLevel InLevel,
        const FGuid& InId,
        UObject* InAsset,
        const FText& InDisplayName,
        const FGameplayTag& InLocationTag,
        const FGameplayTagContainer& InClassificationTags)
    {
        Level = InLevel;
        LocationId = InId;
        LocationAsset = InAsset;
        DisplayName = InDisplayName;
        LocationTag = InLocationTag;
        ClassificationTags = InClassificationTags;
        return this;
    }
};