// InteriorCheckTypes.h (дополнение)
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

// Тип идентификатора объекта для проверки существования/уничтожения
UENUM(BlueprintType)
enum class EObjectIdentifierType : uint8
{
    ItemId      UMETA(DisplayName = "ItemId (GUID)"),
    ActorRef    UMETA(DisplayName = "Actor Reference (AActor*)")
};

// Тип тега
UENUM(BlueprintType)
enum class ETagType : uint8
{
    TextTag     UMETA(DisplayName = "Text Tag (FName)"),
    GameplayTag UMETA(DisplayName = "Gameplay Tag")
};