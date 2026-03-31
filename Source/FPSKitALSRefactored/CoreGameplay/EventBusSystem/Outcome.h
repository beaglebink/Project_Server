#pragma once

// Event type enumeration
// (Перечисление типов событий)
UENUM(BlueprintType)
enum class EOutcomeType : uint8
{
	Default					UMETA(DisplayName = "Default"),
	Mission					UMETA(DisplayName = "Mission"),
	Actor					UMETA(DisplayName = "Actor"),
	Object					UMETA(DisplayName = "Object"),
	Terninal				UMETA(DisplayName = "Terminal"),
	Interior				UMETA(DisplayName = "Interior"),
	SpawnGroup				UMETA(DisplayName = "Spawn Group"),
	WorldState				UMETA(DisplayName = "World state")
};

// Mission category enumeration
// (Перечисление категорий миссий)
UENUM(BlueprintType)
enum class EOutcomeMission : uint8
{
	Default					UMETA(DisplayName = "Default"),
	MissionActivated		UMETA(DisplayName = "Mission Activated"),
	MissionStepCompleted	UMETA(DisplayName = "Mission Step Completed"),
	MissionCompleted		UMETA(DisplayName = "Mission Completed"),
	MissionFailed			UMETA(DisplayName = "Mission Failed"),
	MissionAbandoned		UMETA(DisplayName = "Mission Abandoned")
};

// Actor category enumeration
// (Перечисление категорий актёров)
UENUM(BlueprintType)
enum class EOutcomeActor : uint8
{
	Default					UMETA(DisplayName = "Default"),
	ActorSpawned			UMETA(DisplayName = "On Actor Spawned"),
	ActorDespawned			UMETA(DisplayName = "On Actor Despawned"),
	ChangeAttitude			UMETA(DisplayName = "Change Attitude"),
	DialogueStarted			UMETA(DisplayName = "Dialogue Started"),
	DialogueOptionSelected	UMETA(DisplayName = "Dialogue Option Selected"),
	DialogueEnded			UMETA(DisplayName = "Dialogue Ended")
};

// Object category enumeration
// (Перечисление категорий объектов)
UENUM(BlueprintType)
enum class EOutcomeObject : uint8
{
	Default					UMETA(DisplayName = "Default"),
	InteractionRequest		UMETA(DisplayName = "Interaction Request"),
	InteractionRejected		UMETA(DisplayName = "Interaction Rejected"),
	InteractionStarted		UMETA(DisplayName = "Interaction Started"),
	InteractionCompleted	UMETA(DisplayName = "Interaction Completed"),
	InteractionFailed		UMETA(DisplayName = "Interaction Failed"),
	ObjectSpawned			UMETA(DisplayName = "Object Spawned"),
	ObjectDespawned			UMETA(DisplayName = "Object Despawned"),
	ObjectUsed				UMETA(DisplayName = "Object Used"),
	ObjectDestroyed			UMETA(DisplayName = "Object Destroyed"),
	ItemAcquired			UMETA(DisplayName = "Item Acquired"),
	ItemDelivered			UMETA(DisplayName = "Item Delivered")
};

// Terminal category enumeration
// (Перечисление категорий терминалов)
UENUM(BlueprintType)
enum class EOutcomeTerminal : uint8
{
	Default					UMETA(DisplayName = "Default"),
	TerminalOpened			UMETA(DisplayName = "Terminal Opened"),
	TerminalTaskCompleted	UMETA(DisplayName = "Terminal Task Completed"),
	TerminalTaskFailed		UMETA(DisplayName = "Terminal Task Failed"),
	TerminalClosed			UMETA(DisplayName = "Terminal Closed")
};

// Interior category enumeration
// (Перечисление категорий интерьеров)
UENUM(BlueprintType)
enum class EOutcomeInterior : uint8
{
	Default					UMETA(DisplayName = "Default"),
	InteriorEntered			UMETA(DisplayName = "Interior Entered"),
	InteriorTransition		UMETA(DisplayName = "Interior Transition"),
	InteriorLeaving			UMETA(DisplayName = "Interior Leaving"),
	LocationEntered			UMETA(DisplayName = "Location Entered"),
	LocationTransition		UMETA(DisplayName = "Location Transition"),
	LocationLeaving			UMETA(DisplayName = "Location Leaving"),
	FloorEntered			UMETA(DisplayName = "Floor Entered"),
	FloorTransition			UMETA(DisplayName = "Floor Transition"),
	FloorLeaving			UMETA(DisplayName = "Floor Leaving")
};

// Spawn group category enumeration
// (Перечисление категорий групп спавна)
UENUM(BlueprintType)
enum class EOutcomeSpawnGroup : uint8
{
	Default					UMETA(DisplayName = "Default"),
	SpawnGroupActivated		UMETA(DisplayName = "Spawn Group Activated"),
	SpawnGroupDeactivated	UMETA(DisplayName = "Spawn Group Deactivated"),
	SpawnGroupCleared		UMETA(DisplayName = "Spawn Group Cleared"),
	SpawnGroupEnabled		UMETA(DisplayName = "Spawn Group Enabled"),
	SpawnGroupDisabled		UMETA(DisplayName = "Spawn Group Disabled")
};

// World state category enumeration
// (Перечисление категорий состояния мира)
UENUM(BlueprintType)
enum class EWorldState : uint8
{
	Default								UMETA(DisplayName = "Default"),
	ChangingLocationAvailability		UMETA(DisplayName = "Changing Location Availability"),
	ChangingExteriorDoorAvailability	UMETA(DisplayName = "Changing Exterior Door Availability")
};