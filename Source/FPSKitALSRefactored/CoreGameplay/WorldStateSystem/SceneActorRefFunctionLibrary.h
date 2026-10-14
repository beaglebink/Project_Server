#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SceneActorRef.h"
#include "SceneActorRefFunctionLibrary.generated.h"

class AActor;
class UFloorAsset;
class UWorldRegionAsset;

/**
 * BlueprintPure-фабрики и поиск актёров по ассету сцены.
 *
 * Одна структура FSceneActorRef используется и в InteractSetEnabledPayload,
 * и в WorldStateRecord::ActorRef. Фильтр пикера задаётся метаданными
 * SceneActorFilter на UPROPERTY, на которой висит структура.
 */
UCLASS()
class FPSKITALSREFACTORED_API USceneActorRefFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // ─────────────────────────────────────────────────────────────────────
    // Готовые ссылки на актёра: find + make одной нодой.
    // Отдельные функции на Floor и Region — пины не смешиваются.
    // ─────────────────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Actor",
        meta = (DisplayName = "Find Scene Actor Ref By Label On Floor"))
    static FSceneActorRef FindSceneActorRefByLabelOnFloor(
        UFloorAsset* FloorAsset,
        const FString& ActorLabel);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Actor",
        meta = (DisplayName = "Find Scene Actor Ref By Label On Region"))
    static FSceneActorRef FindSceneActorRefByLabelOnRegion(
        UWorldRegionAsset* RegionAsset,
        const FString& ActorLabel);

    // ─────────────────────────────────────────────────────────────────────
    // Фабрика из AActor* — когда актёр уже известен в графе.
    // ─────────────────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Actor",
        meta = (DisplayName = "Make Scene Actor Ref From Actor"))
    static FSceneActorRef MakeSceneActorRefFromActor(AActor* TargetActor);

    // ─────────────────────────────────────────────────────────────────────
    // Список актёров по ассету сцены.
    // ─────────────────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, Category = "Scene Actor")
    static TArray<AActor*> GetActorsFromFloorAsset(UFloorAsset* FloorAsset, bool bRequireFloorAssignment = true);

    UFUNCTION(BlueprintCallable, Category = "Scene Actor")
    static TArray<AActor*> GetActorsFromRegionAsset(UWorldRegionAsset* RegionAsset, bool bRequireFloorAssignment = true);

    // ─────────────────────────────────────────────────────────────────────
    // Поиск в текущем game world по persistent id (UUID).
    // ─────────────────────────────────────────────────────────────────────
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Scene Actor")
    static AActor* FindActorByPersistentId(const FGuid& PersistentId);
};