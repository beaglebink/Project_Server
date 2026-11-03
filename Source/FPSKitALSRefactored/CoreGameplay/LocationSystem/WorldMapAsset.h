#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
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

    // ── Геймплей-теги ──────────────────────────────────────────────────────
    // Основной тег локации. Задаётся дизайнером в редакторе.
    // Используется в условиях (LocationConditionAsset / ScenarioConditionAsset)
    // и в любой gameplay-логике, которой нужно знать тип локации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги для категоризации (регион пустыни, стартовая карта и т.п.).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Tags")
    FGameplayTagContainer ClassificationTags;

    // ── Глобальные метаданные навигации ────────────────────────────────────
    FVector NavigationOrigin = FVector::ZeroVector;
    float NavigationScale = 1.0f;

    // ── Дочерние регионы (обратные ссылки, заполняются вручную) ────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WorldMap|Regions")
    TArray<TSoftObjectPtr<class UWorldRegionAsset>> Regions;

    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override
    {
        Super::PostDuplicate(DuplicateMode);
        WorldMapID = FGuid::NewGuid();
    }

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("WorldMap", GetFName());
    }
};