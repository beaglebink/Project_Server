#pragma once

// Event type enumeration
UENUM(BlueprintType)
enum class EOutcomeType : uint8
{
	Default               UMETA(DisplayName = "Default"),
	GhostCleared          UMETA(DisplayName = "Ghost Cleared"),
	DialogueStarted       UMETA(DisplayName = "Dialogue Started"),
	TerminalTaskCompleted UMETA(DisplayName = "Terminal Task Completed"),
	MissionCompleted      UMETA(DisplayName = "Mission Completed"),
	ItemAcquired          UMETA(DisplayName = "Item Acquired"),
	MissionProgress       UMETA(DisplayName = "Mission Progress"),
	InteriorTransition    UMETA(DisplayName = "Interior Transition")
};

// Mission category enumeration
UENUM(BlueprintType)
enum class EOutcomeMission : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Actor category enumeration
UENUM(BlueprintType)
enum class EOutcomeActor : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Object category enumeration
UENUM(BlueprintType)
enum class EOutcomeObject : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Interior category enumeration
UENUM(BlueprintType)
enum class EOutcomeInterior : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Spawn group category enumeration
UENUM(BlueprintType)
enum class EOutcomeSpawnGroup : uint8
{
	Default UMETA(DisplayName = "Default")
};