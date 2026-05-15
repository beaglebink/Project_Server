#pragma once

#include "CoreMinimal.h"
#include "FloorPopulationTypes.generated.h"

UENUM(BlueprintType)
enum class EFloorActorType : uint8
{
	HeavyFurniture		UMETA(DisplayName = "Heavy Furniture"),
	LightItem			UMETA(DisplayName = "Light Item"),
	Debris 				UMETA(DisplayName = "Debris"),
	DoorLocks 			UMETA(DisplayName = "Door Locks"),
	StableActor			UMETA(DisplayName = "Stable Actor"),
	DialogueAccess 		UMETA(DisplayName = "Dialogue Access"),
	Terminal			UMETA(DisplayName = "Terminal"),
	InventoryItems 		UMETA(DisplayName = "Inventory Items"),
	NPC_Spawner 		UMETA(DisplayName = "NPC Spawner"),
	LocationTriggers    UMETA(DisplayName = "Location Triggers")
};

// Key: specific building (InteriorSetId) + floor (FloorId)
// (Ключ: конкретное здание (InteriorSetId) + этаж (FloorId))
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FInteriorFloorKey
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FloorKey")
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadOnly, Category = "FloorKey")
	FGuid FloorId;

	FInteriorFloorKey() {}
	FInteriorFloorKey(const FGuid& InInterior, const FGuid& InFloor)
		: InteriorSetId(InInterior), FloorId(InFloor) {}

	bool operator==(const FInteriorFloorKey& Other) const
	{
		return InteriorSetId == Other.InteriorSetId && FloorId == Other.FloorId;
	}
};

// Hash function for the key — combine hashes of GUID components directly
// to avoid ambiguity when resolving GetTypeHash overloads.
// (Хеш-функция для ключа — формируем хеши из компонент GUID напрямую,
// чтобы избежать неоднозначности при разрешении перегрузок GetTypeHash.)
FORCEINLINE uint32 GetTypeHash(const FInteriorFloorKey& K)
{
	auto HashGuid = [](const FGuid& G) -> uint32
	{
		uint32 H = G.A;
		H = HashCombine(H, G.B);
		H = HashCombine(H, G.C);
		H = HashCombine(H, G.D);
		return H;
	};

	const uint32 H1 = HashGuid(K.InteriorSetId);
	const uint32 H2 = HashGuid(K.FloorId);
	return HashCombine(H1, H2);
}

// Record of a placed object (from the level or spawned)
// (Запись о размещённом объекте (из уровня или заспавненном))
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FFloorPopulationRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	EFloorActorType ActorType = EFloorActorType::LightItem;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TSubclassOf<AActor> SourceClass;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TWeakObjectPtr<AActor> PlacedActor;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	FGuid ActorId;

    // World transform of the instance — now store full transform instead of only position
    // (Мировой трансформ экземпляра — теперь сохраняем полный трансформ вместо только позиции)
	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	FTransform WorldTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	bool bHasAnchor = false;
};

// Grouping of arrays by semantic categories for one key (InteriorSet + Floor).
// (Группировка массивов по семантике для одного ключа (InteriorSet + Floor).)
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FFloorPopulationBuckets
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TArray<FFloorPopulationRecord> HeavyFurniture;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TArray<FFloorPopulationRecord> LightItems;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TArray<FFloorPopulationRecord> Terminals;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TArray<FFloorPopulationRecord> NPCSpawners;

	UPROPERTY(BlueprintReadOnly, Category = "FloorPopulation")
	TArray<FFloorPopulationRecord> Debris;
};