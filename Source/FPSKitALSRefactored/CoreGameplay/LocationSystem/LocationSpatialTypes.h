#pragma once

#include "CoreMinimal.h"
#include "LocationTypes.h"
#include "GameplayTagContainer.h" 
#include "LocationSpatialTypes.generated.h"

// ─── Ссылка на целевой якорь (без открытия карты) ──────────────────────────
// Хранит полный путь по иерархии ассетов до конкретного якоря
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationAnchorLink
{
    GENERATED_BODY()

    // Целевой регион (нужен для загрузки карты района)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UWorldRegionAsset> TargetRegion;

    // Целевая улица
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UStreetAsset> TargetStreet;

    // Целевое здание
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UInteriorSetAsset> TargetInteriorSet;

    // Целевой этаж (содержит ссылку на карту)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UFloorAsset> TargetFloor;

    // ID якоря на целевой карте (выбирается из списка якорей TargetFloor/TargetRegion)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    FGuid TargetAnchorID;

    // Кэшированное имя якоря (заполняется утилитой, read-only для удобства)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    FText TargetAnchorDisplayName;

    bool IsValid() const
    {
        return TargetAnchorID.IsValid();
    }
};

// ─── Zone ──────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationZone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Identity")
    FGuid ZoneID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Hierarchy")
    ELocationContextType ParentContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Hierarchy")
    FGuid ParentLocationID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Bounds")
    FVector BoundsOrigin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Bounds")
    FVector BoundsExtent = FVector(200.f, 200.f, 200.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Classification")
    TArray<EZoneTag> Tags;
};

// ─── Anchor ────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationAnchor
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Identity")
    FGuid AnchorID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Hierarchy")
    ELocationContextType ParentContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Hierarchy")
    FGuid ParentLocationID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Transform")
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Transform")
    FRotator WorldOrientation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Classification")
    FGameplayTagContainer Tags;

    // ── Обратная связь: куда ведёт этот якорь при интеракции ───────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Link")
    FLocationAnchorLink ReturnLink;
};

// ─── TransitionPoint ───────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationTransitionPoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Identity")
    FGuid TransitionPointID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    ELocationContextType SourceContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    FGuid SourceLocationID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Destination")
    ELocationContextType DestinationContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Destination")
    FGuid DestinationLocationID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Transform")
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Transform")
    FRotator WorldOrientation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Classification")
    ELocationTransitionType TransitionType = ELocationTransitionType::Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Metadata")
    FText AddressLabel;
};