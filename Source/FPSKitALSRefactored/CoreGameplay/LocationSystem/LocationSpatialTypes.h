#pragma once

#include "CoreMinimal.h"
#include "LocationTypes.h"
#include "GameplayTagContainer.h"
#include "LocationSpatialTypes.generated.h"

// Убираем forward-декларацию ALocationAnchorActor — больше не нужна в заголовке.

// ─── Ссылка на целевой якорь ───────────────────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationAnchorLink
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UWorldRegionAsset> TargetRegion;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UStreetAsset> TargetStreet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UInteriorSetAsset> TargetInteriorSet;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<class UFloorAsset> TargetFloor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    FGuid TargetAnchorID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    FText TargetAnchorDisplayName;

    // Soft-ссылка на актор назначения — храним как UObject чтобы не требовать полного типа в заголовке.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AnchorLink")
    TSoftObjectPtr<UObject> TargetAnchorActor;

    bool IsValid() const { return TargetAnchorID.IsValid(); }
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Identity")
    FGuid TransitionPointID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Identity")
    FText DisplayName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Classification")
    ELocationTransitionType TransitionType = ELocationTransitionType::Default;

    // ── Источник: сохраняем soft ptr как UObject (без необходимости полного типа здесь)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    TSoftObjectPtr<UObject> SourceAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    FVector SourceWorldPosition = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    FRotator SourceWorldOrientation = FRotator::ZeroRotator;

    // ── Назначение: soft ptr к объекту назначения (как UObject), и полный DestinationLink
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Destination")
    TSoftObjectPtr<UObject> DestinationAnchor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Destination")
    FLocationAnchorLink DestinationLink;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition|Metadata")
    FText AddressLabel;
};