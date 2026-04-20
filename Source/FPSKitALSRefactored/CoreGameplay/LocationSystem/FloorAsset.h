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
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Identity")
    FGuid FloorID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Identity")
    FText DisplayName;

    // Порядковый номер этажа (0 = первый, отрицательные — подвалы)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Identity")
    int32 FloorIndex = 0;

    // ── Уровень (сцена) для SeamlessTravel ────────────────────────────────
    // Путь к карте которая загружается при переходе на этаж
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Level")
    TSoftObjectPtr<UWorld> FloorLevel;

    // ── Иерархия ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Hierarchy")
    TSoftObjectPtr<class UInteriorSetAsset> ParentInteriorSet;

    // ── Навигация ──────────────────────────────────────────────────────────
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

    // ── Пространственные элементы ──────────────────────────────────────────
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Spatial")
    TArray<FLocationZone> Zones;

    // Якоря входа на этаж — используются для позиционирования игрока после загрузки
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Spatial")
    TArray<FLocationAnchor> Anchors;

    // Переходы: лестницы, лифты, двери между этажами и на улицу
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Spatial")
    TArray<FLocationTransitionPoint> TransitionPoints;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Floor", GetFName());
    }
};