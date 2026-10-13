#pragma once

#include "CoreMinimal.h"
#include "SceneActorRef.h"  // для ESceneAssetKind
#include "WorldStateActorRef.generated.h"

class UFloorAsset;
class UWorldRegionAsset;

/**
 * Ссылка на актёра сцены, используемая в FWorldStateRecord.
 *
 * Отличие от FSceneActorRef: пикер в редакторе фильтрует актёров только
 * по наличию UFloorAssignmentComponent (FSceneActorRef дополнительно
 * требует UInteractiveItemComponent).
 *
 * Хранит ActorPersistentId — GUID из UFloorAssignmentComponent::ItemId.
 * Он стабилен, сохраняется в .uasset и попадает в сейв.
 */
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FWorldStateActorRef
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World State Actor")
    ESceneAssetKind SceneKind = ESceneAssetKind::Floor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World State Actor",
        meta = (EditCondition = "SceneKind == ESceneAssetKind::Floor",
            EditConditionHides))
    TSoftObjectPtr<UFloorAsset> SceneFloor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World State Actor",
        meta = (EditCondition = "SceneKind == ESceneAssetKind::Region",
            EditConditionHides))
    TSoftObjectPtr<UWorldRegionAsset> SceneRegion;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "World State Actor")
    FGuid ActorPersistentId;

    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "World State Actor")
    FString ActorLabel;

    bool IsValid() const
    {
        const bool bHasScene =
            (SceneKind == ESceneAssetKind::Floor && !SceneFloor.IsNull()) ||
            (SceneKind == ESceneAssetKind::Region && !SceneRegion.IsNull());

        return bHasScene && ActorPersistentId.IsValid();
    }
};