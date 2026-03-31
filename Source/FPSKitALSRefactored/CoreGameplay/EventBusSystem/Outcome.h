#pragma once

// Event type enumeration
UENUM(BlueprintType)
enum class EOutcomeType : uint8
{
	Default					UMETA(DisplayName = "Default", Hidden),
	Mission					UMETA(DisplayName = "Mission"),
	Actor					UMETA(DisplayName = "Actor"),
	Object					UMETA(DisplayName = "Object"),
	Interior				UMETA(DisplayName = "Interior"),
	SpawnGroup				UMETA(DisplayName = "Spawn Group")
};

// Mission category enumeration
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
// Added BlueprintType and uint8 for use in conditions and Blueprint
// (ƒобавлен BlueprintType и uint8 дл€ использовани€ в услови€х и Blueprint)
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
// Added BlueprintType and uint8 for use in conditions and Blueprint
// (ƒобавлен BlueprintType и uint8 дл€ использовани€ в услови€х и Blueprint)
UENUM(BlueprintType)
enum class EWorldState : uint8
{
	Default								UMETA(DisplayName = "Default"),
	ChangingLocationAvailability		UMETA(DisplayName = "Changing Location Availability"),
	ChangingExteriorDoorAvailability	UMETA(DisplayName = "Changing Exterior Door Availability")
};