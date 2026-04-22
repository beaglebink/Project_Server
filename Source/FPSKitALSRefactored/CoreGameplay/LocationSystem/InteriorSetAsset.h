#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LocationTypes.h"
#include "LocationSpatialTypes.h"
#include "InteriorSetAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorSetAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    FGuid InteriorSetID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Hierarchy")
    TSoftObjectPtr<class UStreetAsset> ParentStreet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Address")
    FText AddressLine;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Address")
    int32 AddressNumber = 0;

    TArray<EInteriorSetFlag> ClassificationFlags;

    // Точки перехода в здании (входы) — видны в Editor, из BP только для чтения.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Transitions")
    TArray<FLocationTransitionPoint> TransitionPoints;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Floors")
    TArray<TSoftObjectPtr<class UFloorAsset>> Floors;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("InteriorSet", GetFName());
    }
};