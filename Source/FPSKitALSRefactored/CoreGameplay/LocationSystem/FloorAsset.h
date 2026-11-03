#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/SoftObjectPtr.h"
#include "GameplayTagContainer.h"
#include "LocationSpatialTypes.h"
#include "FloorAsset.generated.h"

UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UFloorAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    FGuid FloorID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    int32 FloorIndex = 0;

    // ── Геймплей-теги ──────────────────────────────────────────────────────
    // Основной тег локации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Tags")
    FGameplayTag LocationTag;

    // Дополнительные теги для категоризации.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Tags")
    FGameplayTagContainer ClassificationTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Level")
    TSoftObjectPtr<UWorld> FloorLevel;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Hierarchy")
    TSoftObjectPtr<class UInteriorSetAsset> ParentInteriorSet;

    // Кэш идентификатора родительского InteriorSetAsset.
    // Заполняется автоматически в редакторе и при загрузке.
    UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Floor|Identity")
    FGuid InteriorSetID;

    FVector NavigationOrigin = FVector::ZeroVector;
    TArray<FLocationZone> Zones;
    TArray<FLocationAnchor> Anchors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor|Transitions")
    TArray<FLocationTransitionPoint> TransitionPoints;

    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override
    {
        Super::PostDuplicate(DuplicateMode);
        FloorID = FGuid::NewGuid();
    }

    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostInitProperties() override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId("Floor", GetFName());
    }
};