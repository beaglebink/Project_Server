#pragma once

// Event type enumeration (Перечисление типов событий)
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

// Mission category enumeration (Перечисление категории миссии)
UENUM()
enum class EOutcomeMission : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Actor category enumeration (Перечисление категории актера)
UENUM()
enum class EOutcomeActor : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Object category enumeration (Перечисление категории объекта)
UENUM()
enum class EOutcomeObject : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Interior category enumeration (Перечисление категории интерьера)
UENUM()
enum class EOutcomeInterior : uint8
{
	Default UMETA(DisplayName = "Default")
};

// Spawn group category enumeration (Перечисление категории группы спауна)
UENUM()
enum class EOutcomeSpawnGroup : uint8
{
	Default UMETA(DisplayName = "Default")
};