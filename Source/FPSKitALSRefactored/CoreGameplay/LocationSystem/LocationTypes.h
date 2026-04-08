#pragma once

#include "CoreMinimal.h"
#include "LocationTypes.generated.h"

// ─── Классификация перехода (переименовано, чтобы избежать конфликта с движком) ────────────────────────────────────────────────
UENUM(BlueprintType)
enum class ELocationTransitionType : uint8
{
    Default         UMETA(DisplayName = "Default"),
    Door            UMETA(DisplayName = "Door"),
    Elevator        UMETA(DisplayName = "Elevator"),
    Stairs          UMETA(DisplayName = "Stairs"),
    Ladder          UMETA(DisplayName = "Ladder"),
    WorldBoundary   UMETA(DisplayName = "World Boundary")
};

// ─── Источник (тип родительской локации) ──────────────────────────────────
UENUM(BlueprintType)
enum class ELocationContextType : uint8
{
    None            UMETA(DisplayName = "None"),
    Street          UMETA(DisplayName = "Street"),
    Floor           UMETA(DisplayName = "Floor")
};

// ─── Тег классификации зоны ────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EZoneTag : uint8
{
    Generic         UMETA(DisplayName = "Generic"),
    Sidewalk        UMETA(DisplayName = "Sidewalk"),
    Roadway         UMETA(DisplayName = "Roadway"),
    Room            UMETA(DisplayName = "Room"),
    Corridor        UMETA(DisplayName = "Corridor"),
    Lobby           UMETA(DisplayName = "Lobby"),
    Stairwell       UMETA(DisplayName = "Stairwell"),
    Restricted      UMETA(DisplayName = "Restricted")
};

// ─── Флаги классификации InteriorSet ──────────────────────────────────────
UENUM(BlueprintType)
enum class EInteriorSetFlag : uint8
{
    Generic         UMETA(DisplayName = "Generic"),
    Residential     UMETA(DisplayName = "Residential"),
    Commercial      UMETA(DisplayName = "Commercial"),
    Industrial      UMETA(DisplayName = "Industrial"),
    Government      UMETA(DisplayName = "Government"),
    Underground     UMETA(DisplayName = "Underground")
};

// ─── Адресная строка, собираемая утилитой ─────────────────────────────────
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FLocationAddress
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Location|Address")
    FText WorldMapName;

    UPROPERTY(BlueprintReadOnly, Category = "Location|Address")
    FText RegionName;

    UPROPERTY(BlueprintReadOnly, Category = "Location|Address")
    FText StreetName;

    UPROPERTY(BlueprintReadOnly, Category = "Location|Address")
    FText BuildingName;

    UPROPERTY(BlueprintReadOnly, Category = "Location|Address")
    FText FloorName;

    UPROPERTY(BlueprintReadOnly, Category = "Location|Address")
    FText ZoneName;

    // Возвращает одну читаемую строку вида "RegionName, StreetName, BuildingName, FloorName, ZoneName"
    FString ToDisplayString() const;
};