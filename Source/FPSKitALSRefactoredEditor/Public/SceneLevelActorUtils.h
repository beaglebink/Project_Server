#pragma once

#include "CoreMinimal.h"
#include "SceneActorRef.h"
#include "UObject/SoftObjectPath.h"

class AActor;
class UWorld;
class UFloorAsset;
class UWorldRegionAsset;

/**
 * Editor-side utility: сканирование актёров на уровне, на который ссылается
 * FloorAsset / RegionAsset. Работает и для anchor'ов, и для scene-актёров.
 *
 * Не UCLASS — простой C++-хелпер, живёт только в editor-модуле.
 */
class FSceneLevelActorUtils
{
public:
    // Резолвит UWorld по пути к пакету уровня. Грузит пакет, если он не загружен.
    // Возвращает nullptr, если пакет не найден или мир невалиден.
    static UWorld* ResolveWorldFromLevelPath(const FSoftObjectPath& LevelPath);

    // Резолвит UWorld по паре (SceneKind, Floor, Region).
    static UWorld* ResolveWorldFromSceneRef(
        ESceneAssetKind Kind,
        const TSoftObjectPtr<UFloorAsset>& Floor,
        const TSoftObjectPtr<UWorldRegionAsset>& Region);

    // Возвращает всех актёров с PersistentLevel, для которых Predicate(Actor) == true.
    // Streaming sublevels игнорируются — как в вашем ScanLevelForAnchors.
    static void CollectActors(
        UWorld* World,
        TFunctionRef<bool(AActor*)> Predicate,
        TArray<AActor*>& OutActors);

    // Готовые предикаты.
    static bool HasFloorAssignment(AActor* Actor);
    static bool HasInteractiveItem(AActor* Actor);
};