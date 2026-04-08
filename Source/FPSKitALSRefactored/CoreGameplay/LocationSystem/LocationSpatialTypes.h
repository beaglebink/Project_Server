#pragma once

#include "CoreMinimal.h"
#include "LocationTypes.h"
#include "GameplayTagContainer.h" 
#include "LocationSpatialTypes.generated.h"

// ─── Zone ──────────────────────────────────────────────────────────────────
// Представляет область (комната, коридор, тротуар и т.д.)
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationZone
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Identity")
    FGuid ZoneID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Identity")
    FText DisplayName;

    // Тип родительской локации: Street или Floor
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Hierarchy")
    ELocationContextType ParentContextType = ELocationContextType::None;

    // ID родителя (Street или Floor — определяется контекстом)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Hierarchy")
    FGuid ParentLocationID;

    // Ограничивающий объём (центр + экстент в локальных координатах)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Bounds")
    FVector BoundsOrigin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Bounds")
    FVector BoundsExtent = FVector(200.f, 200.f, 200.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Classification")
    TArray<EZoneTag> Tags;
};

// ─── Anchor ────────────────────────────────────────────────────────────────
// Представляет точную точку с позицией и ориентацией
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

    // Мировые координаты
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Transform")
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Transform")
    FRotator WorldOrientation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Anchor|Classification")
    FGameplayTagContainer Tags;
};

// ─── TransitionPoint ───────────────────────────────────────────────────────
// Соединяет два пространственных узла (двери, лифты, лестницы, границы карт)
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationTransitionPoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Identity")
    FGuid TransitionPointID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Identity")
    FText DisplayName;

    // ── Источник ──────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    ELocationContextType SourceContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Source")
    FGuid SourceLocationID;

    // ── Назначение ────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Destination")
    ELocationContextType DestinationContextType = ELocationContextType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Destination")
    FGuid DestinationLocationID;

    // ── Позиция в мире ────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Transform")
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Transform")
    FRotator WorldOrientation = FRotator::ZeroRotator;

    // ── Классификация ─────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Classification")
    ELocationTransitionType TransitionType = ELocationTransitionType::Default;

    // Опциональный адресный тег (например, "Главный вход")
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Transition|Metadata")
    FText AddressLabel;
};