// InteriorCheckTypes.h
#pragma once

#include "CoreMinimal.h"
#include "FloorPopulationTypes.h"
#include "InteriorCheckTypes.generated.h"

UENUM(BlueprintType)
enum class EInteriorActorCountType : uint8
{
    Registered  UMETA(DisplayName = "Registered Actors (Level + Global Spawns)"),
    Spawned     UMETA(DisplayName = "Spawned by Missions"),
    Destroyed   UMETA(DisplayName = "Destroyed by Missions")
};