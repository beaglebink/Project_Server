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
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Identity")
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

    // ── Дочерние улицы ─────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Streets")
    TArray<TSoftObjectPtr<class UStreetAsset>> Streets;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("WorldRegion", GetFName());
    }
};