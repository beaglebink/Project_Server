#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "LocationTypes.h"
#include "LocationSpatialTypes.h"
#include "InteriorSetAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteriorSetAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteriorSet|Identity")
    FGuid InteriorSetID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteriorSet|Identity")
    FText DisplayName;

    // ── Геймплей-теги ──────────────────────────────────────────────────────
    // Основной тег локации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги для категоризации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Tags")
    FGameplayTagContainer ClassificationTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteriorSet|Hierarchy")
    TSoftObjectPtr<class UStreetAsset> ParentStreet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteriorSet|Address")
    FText AddressLine;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteriorSet|Address")
    int32 AddressNumber = 0;

    // Легаси-флаги категоризации. Оставлены для совместимости с существующими
    // ассетами; новую логику лучше строить на ClassificationTags / LocationTag.
    TArray<EInteriorSetFlag> ClassificationFlags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteriorSet|Transitions")
    TArray<FLocationTransitionPoint> TransitionPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InteriorSet|Floors")
    TArray<TSoftObjectPtr<class UFloorAsset>> Floors;

    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override
    {
        Super::PostDuplicate(DuplicateMode);
        InteriorSetID = FGuid::NewGuid();
    }

#if WITH_EDITOR
    virtual void PostInitProperties() override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("InteriorSet", GetFName());
    }
};