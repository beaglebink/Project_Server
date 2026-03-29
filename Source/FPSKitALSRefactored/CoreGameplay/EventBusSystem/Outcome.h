#pragma once

UENUM()
enum class EOutcomeType : uint8
{
	Default UMETA(DisplayName = "Default"),
	GhostCleared UMETA(DisplayName = "Ghost Cleared"),
	DialogueStarted UMETA(DisplayName = "Dialogue Started"),
	TerminalTaskCompleted UMETA(DisplayName = "Terminal Task Completed"),
	MissionCompleted UMETA(DisplayName = "Mission Completed"),
	ItemAcquired UMETA(DisplayName = "Item Acquired")
};

UENUM()
enum class EOutcomeMission : uint8
{
	Default UMETA(DisplayName = "Default")
};

UENUM()
enum class EOutcomeActor : uint8
{
	Default UMETA(DisplayName = "Default")
};

UENUM()
enum class EOutcomeObject : uint8
{
	Default UMETA(DisplayName = "Default")
};

UENUM()
enum class EOutcomeInterior : uint8
{
	Default UMETA(DisplayName = "Default")
};

UENUM()
enum class EOutcomeSpawnGroup : uint8
{
	Default UMETA(DisplayName = "Default")
};