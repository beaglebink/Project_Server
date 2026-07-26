// Outcome.h
#pragma once

UENUM(BlueprintType)
enum class EOutcomeType : uint8
{
    Default         UMETA(DisplayName = "Default"),
    Mission         UMETA(DisplayName = "Mission"),
    Actor           UMETA(DisplayName = "Actor"),
    Inventory       UMETA(DisplayName = "Inventory"),
    Terminal        UMETA(DisplayName = "Terminal"),
    Interior        UMETA(DisplayName = "Interior"),
    SpawnGroup      UMETA(DisplayName = "Spawn Group"),
    WorldState      UMETA(DisplayName = "World state")
};

UENUM(BlueprintType)
enum class EOutcomeMission : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    MissionActivated        UMETA(DisplayName = "Mission Activated"),
    MissionUpdate           UMETA(DisplayName = "Mission Update"),
    MissionProgress         UMETA(DisplayName = "Mission Progress"),
    MissionCompleted        UMETA(DisplayName = "Mission Completed"),
    MissionFailed           UMETA(Hidden, DisplayName = "Mission Failed"),
    MissionAbandoned        UMETA(Hidden, DisplayName = "Mission Abandoned"),
    CheckRequest            UMETA(DisplayName = "Check Request"),
    CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeActor : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    ActorSpawned            UMETA(DisplayName = "On Actor Spawned"),
    ActorDespawned          UMETA(DisplayName = "On Actor Despawned"),
    ChangeAttitude          UMETA(DisplayName = "Change Attitude"),
    DialogueStarted         UMETA(DisplayName = "Dialogue Started"),
    DialogueOptionSelected  UMETA(DisplayName = "Dialogue Option Selected"),
    DialogueEnded           UMETA(DisplayName = "Dialogue Ended"),
    InteractSetEnabled      UMETA(DisplayName = "Interact Set Enabled"),
    InteractSetRange        UMETA(DisplayName = "Interact Set Range"),
    InteractSetTooltip      UMETA(DisplayName = "Interact Set Tooltip"),
    InteractRegistered      UMETA(Hidden, DisplayName = "Interact Registered"),
    InteractUnregistered    UMETA(Hidden, DisplayName = "Interact Unregistered"),
    CheckRequest            UMETA(DisplayName = "Check Request"),
    CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeInventory : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    InteractionRequest      UMETA(DisplayName = "Interaction Request"),
    InteractionRejected     UMETA(DisplayName = "Interaction Rejected"),
    InteractionStarted      UMETA(DisplayName = "Interaction Started"),
    InteractionCompleted    UMETA(DisplayName = "Interaction Completed"),
    InteractionFailed       UMETA(DisplayName = "Interaction Failed"),
    ObjectSpawned           UMETA(DisplayName = "Object Spawned"),
    ObjectDespawned         UMETA(DisplayName = "Object Despawned"),
    ObjectUsed              UMETA(DisplayName = "Object Used"),
    ObjectDestroyed         UMETA(DisplayName = "Object Destroyed"),
    ItemAcquired            UMETA(DisplayName = "Item Acquired"),
    ItemDelivered           UMETA(DisplayName = "Item Delivered"),
    InteractSetEnabled      UMETA(DisplayName = "Interact Set Enabled"),
    InteractSetRange        UMETA(DisplayName = "Interact Set Range"),
    InteractSetTooltip      UMETA(DisplayName = "Interact Set Tooltip"),
    InteractRegistered      UMETA(Hidden, DisplayName = "Interact Registered"),
    InteractUnregistered    UMETA(Hidden, DisplayName = "Interact Unregistered"),
    CheckRequest            UMETA(DisplayName = "Check Request"),
    CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeTerminal : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    TerminalOpened          UMETA(DisplayName = "Terminal Opened"),
    TerminalTaskCompleted   UMETA(DisplayName = "Terminal Task Completed"),
    TerminalTaskFailed      UMETA(DisplayName = "Terminal Task Failed"),
    TerminalClosed          UMETA(DisplayName = "Terminal Closed"),
    TerminalLoginFailed     UMETA(DisplayName = "Terminal Login Failed"),
    TerminalLoginSucceeded  UMETA(DisplayName = "Terminal Login Succeeded"),
    TerminalFileModified    UMETA(DisplayName = "Terminal File Modified"),
    TerminalGlobalServiceModified UMETA(DisplayName = "Terminal Global Service Modified"),
    InteractSetEnabled      UMETA(DisplayName = "Interact Set Enabled"),
    InteractSetRange        UMETA(DisplayName = "Interact Set Range"),
    InteractSetTooltip      UMETA(DisplayName = "Interact Set Tooltip"),
    TerminalCommand         UMETA(DisplayName = "Terminal command"),
    InteractRegistered      UMETA(Hidden, DisplayName = "Interact Registered"),
    InteractUnregistered    UMETA(Hidden, DisplayName = "Interact Unregistered"),
    CheckRequest            UMETA(DisplayName = "Check Request"),
    CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeInterior : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    InteriorEntered         UMETA(DisplayName = "Interior Entered"),
    InteriorTransition      UMETA(DisplayName = "Interior Transition"),
    InteriorLeaving         UMETA(DisplayName = "Interior Leaving"),
    FloorEntered            UMETA(DisplayName = "Floor Entered"),
    FloorTransition         UMETA(DisplayName = "Floor Transition"),
    FloorLeaving            UMETA(DisplayName = "Floor Leaving"),
    BuildingEntered         UMETA(DisplayName = "Location Entered"),
    BuildingTransition      UMETA(DisplayName = "Location Transition"),
    BuildingLeaving         UMETA(DisplayName = "Location Leaving"),
    InteractSetEnabled      UMETA(DisplayName = "Interact Set Enabled"),
    InteractSetRange        UMETA(DisplayName = "Interact Set Range"),
    InteractSetTooltip      UMETA(DisplayName = "Interact Set Tooltip"),
    InteractRegistered      UMETA(Hidden, DisplayName = "Interact Registered"),
    InteractUnregistered    UMETA(Hidden, DisplayName = "Interact Unregistered"),
    FloorPlacementRegistered    UMETA(Hidden, DisplayName = "Floor Placement Registered"),
    FloorPlacementUnregistered  UMETA(Hidden, DisplayName = "Floor Placement Unregistered"),
    FloorStateSave          UMETA(DisplayName = "Floor State Save"),
    FloorStateRestore       UMETA(DisplayName = "Floor State Restore"),
    LevelLoaded             UMETA(DisplayName = "Level Loaded"),
    CheckRequest            UMETA(DisplayName = "Check Request"),
    CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeSpawnGroup : uint8
{
    Default                 UMETA(Hidden, DisplayName = "Default"),
    SpawnGroupActivated     UMETA(DisplayName = "Spawn Group Activated"),
    SpawnGroupDeactivated   UMETA(DisplayName = "Spawn Group Deactivated"),
    SpawnGroupCleared       UMETA(DisplayName = "Spawn Group Cleared"),
    SpawnGroupEnabled       UMETA(DisplayName = "Spawn Group Enabled"),
    SpawnGroupDisabled      UMETA(DisplayName = "Spawn Group Disabled"),
    SpawnGroupReset         UMETA(DisplayName = "Spawn Group Reset"),
    SpawnGroupRegister      UMETA(DisplayName = "Spawn Group Register"),
    SpawnGroupUnregister    UMETA(DisplayName = "Spawn Group Unregister"),
    GhostCaptured           UMETA(DisplayName = "Ghost Captured"),
    CheckRequest            UMETA(DisplayName = "Check Request"),
    CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EWorldState : uint8
{
    Default                     UMETA(DisplayName = "Default"),
    ChangingLocationAvailability    UMETA(DisplayName = "Changing Location Availability"),
    ChangingExteriorDoorAvailability UMETA(DisplayName = "Changing Exterior Door Availability"),
    CheckRequest                UMETA(DisplayName = "Check Request"),
    CheckResponse               UMETA(DisplayName = "Check Response")
};