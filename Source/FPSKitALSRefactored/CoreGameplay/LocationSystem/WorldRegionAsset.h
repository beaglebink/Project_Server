#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WorldRegionAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UWorldRegionAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Identity")
    FGuid WorldRegionID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Identity")
    FText DisplayName;

    // ── Иерархия ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Hierarchy")
    TSoftObjectPtr<class UWorldMapAsset> ParentWorldMap;

    // ── Классификационные теги ─────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Classification")
    FGameplayTagContainer ClassificationTags;

    // ── Метаданные навигации ───────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldRegion|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

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