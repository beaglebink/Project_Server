#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WorldMapAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UWorldMapAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // ── Идентификатор ──────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WorldMap|Identity")
    FGuid WorldMapID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Identity")
    FText DisplayName;

    // ── Глобальные метаданные навигации ────────────────────────────────────
    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Navigation")
    FVector NavigationOrigin = FVector::ZeroVector;

    //UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Navigation")
    float NavigationScale = 1.0f;

    // ── Дочерние регионы (обратные ссылки, заполняются вручную) ────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Regions")
    TArray<TSoftObjectPtr<class UWorldRegionAsset>> Regions;

#if WITH_EDITOR
    // Автогенерация GUID при создании ассета
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("WorldMap", GetFName());
    }
};