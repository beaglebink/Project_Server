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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Street|Identity")
    FGuid StreetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Street|Identity")
    FText DisplayName;

    // ── Геймплей-теги ──────────────────────────────────────────────────────
    // Основной тег локации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги для категоризации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Tags")
    FGameplayTagContainer ClassificationTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Hierarchy")
    TSoftObjectPtr<class UWorldRegionAsset> ParentWorldRegion;

    FText SubRegionName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

    TArray<FLocationZone> Zones;
    TArray<FLocationAnchor> Anchors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Street|Transitions")
    TArray<FLocationTransitionPoint> TransitionPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Street|Buildings")
    TArray<TSoftObjectPtr<class UInteriorSetAsset>> InteriorSets;

    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override
    {
        Super::PostDuplicate(DuplicateMode);
        StreetID = FGuid::NewGuid();
    }

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Street", GetFName());
    }
};