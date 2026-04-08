#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LocationSpatialTypes.h"
#include "FloorAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UFloorAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Identity")
    FGuid FloorID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Identity")
    FText DisplayName;

    // Порядковый номер этажа (0 = первый, отрицательные — подвалы)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Identity")
    int32 FloorIndex = 0;

    // ── Иерархия ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Hierarchy")
    TSoftObjectPtr<class UInteriorSetAsset> ParentInteriorSet;

    // ── Навигация ──────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

    // ── Пространственные элементы ──────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Spatial")
    TArray<FLocationZone> Zones;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Spatial")
    TArray<FLocationAnchor> Anchors;

    // Переходы: лестницы, лифты, двери между этажами и на улицу
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Spatial")
    TArray<FLocationTransitionPoint> TransitionPoints;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Floor", GetFName());
    }
};