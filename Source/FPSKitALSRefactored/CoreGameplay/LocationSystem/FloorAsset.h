#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"
#include "LocationSpatialTypes.h"
#include "FloorAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UFloorAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Стабильный идентификатор этажа. UPROPERTY обязателен — сохраняется в .uasset
    // и остаётся неизменным между сессиями редактора.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    FGuid FloorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    int32 FloorIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Level")
    TSoftObjectPtr<UWorld> FloorLevel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Hierarchy")
    TSoftObjectPtr<class UInteriorSetAsset> ParentInteriorSet;

    FVector NavigationOrigin = FVector::ZeroVector;
    TArray<FLocationZone> Zones;
    TArray<FLocationAnchor> Anchors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Transitions")
    TArray<FLocationTransitionPoint> TransitionPoints;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Floor", GetFName());
    }
};