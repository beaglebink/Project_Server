#pragma once

#include "CoreMinimal.h"
#include "SceneActorRef.generated.h"

class UFloorAsset;
class UWorldRegionAsset;

UENUM(BlueprintType)
enum class ESceneAssetKind : uint8
{
    Floor   UMETA(DisplayName = "Floor"),
    Region  UMETA(DisplayName = "Region")
};

/**
 * Цель — актёр на сцене. Сцена задаётся одним из двух способов:
 *   - Floor  → берём UFloorAsset::FloorLevel;
 *   - Region → берём UWorldRegionAsset::RegionLevel.
 *
 * Редакторный пикер резолвит уровень из выбранного ассета и предлагает
 * только тех актёров, у которых есть оба компонента — FloorAssignmentComponent
 * (для persistent ID) и InteractiveItemComponent (для самой интеракции).
 *
 * Persistent ID — это UFloorAssignmentComponent::ItemId. Он стабилен и
 * сохраняется в .uasset, поэтому подходит для хранения в payload.
 */
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FSceneActorRef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Actor")
    ESceneAssetKind SceneKind = ESceneAssetKind::Floor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Actor",
        meta = (EditCondition = "SceneKind == ESceneAssetKind::Floor",
            EditConditionHides))
    TSoftObjectPtr<UFloorAsset> SceneFloor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scene Actor",
        meta = (EditCondition = "SceneKind == ESceneAssetKind::Region",
            EditConditionHides))
    TSoftObjectPtr<UWorldRegionAsset> SceneRegion;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Scene Actor")
    FGuid ActorPersistentId;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Scene Actor")
    FString ActorLabel;

    bool IsValid() const
    {
        const bool bHasScene =
            (SceneKind == ESceneAssetKind::Floor && !SceneFloor.IsNull()) ||
            (SceneKind == ESceneAssetKind::Region && !SceneRegion.IsNull());

        return bHasScene && ActorPersistentId.IsValid();
    }
};