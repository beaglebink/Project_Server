#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LocationSpatialTypes.h"
#include "WorldRegionAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UWorldRegionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldRegion|Identity")
    FGuid WorldRegionID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Identity")
    FText DisplayName;

    // ── Иерархия ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Hierarchy")
    TSoftObjectPtr<class UWorldMapAsset> ParentWorldMap;

    // ── Классификационные теги ─────────────────────────────────────────────
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Classification")
    FGameplayTagContainer ClassificationTags;

    // ── Карта региона ──────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Level")
    TSoftObjectPtr<UWorld> RegionLevel;

    // ── Навигация ──────────────────────────────────────────────────────────
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

    // ── Якоря на карте региона (точки выхода из зданий, переходы между районами) ──
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Spatial")
    TArray<FLocationAnchor> Anchors;

    // ── Точки перехода уровня района (акторы без улицы и дома) ────────────
    // Заполняются автоматически при нажатии Apply на ALocationAnchorActor,
    // у которого не указаны OwnerStreet и OwnerInteriorSet.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Transitions")
    TArray<FLocationTransitionPoint> TransitionPoints;

    // ── Дочерние улицы ─────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldRegion|Streets")
    TArray<TSoftObjectPtr<class UStreetAsset>> Streets;

    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override
    {
        Super::PostDuplicate(DuplicateMode);
        WorldRegionID = FGuid::NewGuid();
    }

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("WorldRegion", GetFName());
    }
};