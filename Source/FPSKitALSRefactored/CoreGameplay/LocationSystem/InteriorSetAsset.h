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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Identity")
    FGuid InteriorSetID;

    // Опционально: для общих/безымянных зданий можно оставить пустым
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Identity")
    FText DisplayName;

    // ── Иерархия ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Hierarchy")
    TSoftObjectPtr<class UStreetAsset> ParentStreet;

    // ── Адрес ──────────────────────────────────────────────────────────────
    // Читаемый адрес (например, "ул. Ленина, 12")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Address")
    FText AddressLine;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Address")
    int32 AddressNumber = 0;

    // ── Классификация ──────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Classification")
    TArray<EInteriorSetFlag> ClassificationFlags;

    // ── Входные переходы (двери на улицу) ──────────────────────────────────
    // TransitionPoints, классифицированные как входы в здание
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Entrances")
    TArray<FLocationTransitionPoint> EntranceTransitionPoints;

    // ── Этажи ──────────────────────────────────────────────────────────────
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