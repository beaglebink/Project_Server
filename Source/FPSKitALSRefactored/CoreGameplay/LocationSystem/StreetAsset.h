#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LocationSpatialTypes.h"
#include "StreetAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UStreetAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Identity")
    FGuid StreetID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Identity")
    FText DisplayName;

    // ── Иерархия ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Hierarchy")
    TSoftObjectPtr<class UWorldRegionAsset> ParentWorldRegion;

    // Опциональная классификация суб-района (например, "Портовый квартал")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Classification")
    FText SubRegionName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Classification")
    FGameplayTagContainer ClassificationTags;

    // ── Навигация ──────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

    // ── Пространственные элементы ──────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Spatial")
    TArray<FLocationZone> Zones;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Spatial")
    TArray<FLocationAnchor> Anchors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Spatial")
    TArray<FLocationTransitionPoint> TransitionPoints;

    // ── Здания на улице ────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Buildings")
    TArray<TSoftObjectPtr<class UInteriorSetAsset>> InteriorSets;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Street", GetFName());
    }
};