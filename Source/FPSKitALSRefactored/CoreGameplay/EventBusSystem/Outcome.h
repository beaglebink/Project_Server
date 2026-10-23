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
    WorldState      UMETA(DisplayName = "World state"),
    Chore           UMETA(DisplayName = "Chore")
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
    CheckResponse           UMETA(DisplayName = "Check Response"),
    ChoreStepRequest        UMETA(DisplayName = "Chore Step Request")
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

    // ========================================================================
    // Нотификации подсистемы о состоянии терминала
    // ========================================================================
    TerminalOpened          UMETA(DisplayName = "Terminal Opened"),
    //TerminalTaskCompleted   UMETA(DisplayName = "Terminal Task Completed"),
    TerminalClosed          UMETA(DisplayName = "Terminal Closed"),
    TerminalLoginFailed     UMETA(DisplayName = "Terminal Login Failed"),
    TerminalLoginSucceeded  UMETA(DisplayName = "Terminal Login Succeeded"),
    TerminalFileModified    UMETA(DisplayName = "Terminal File Modified"),

    // ========================================================================
    // Interact-события (взаимодействие с предметом)
    // ========================================================================
    InteractSetEnabled      UMETA(DisplayName = "Interact Set Enabled"),
    InteractSetRange        UMETA(DisplayName = "Interact Set Range"),
    InteractSetTooltip      UMETA(DisplayName = "Interact Set Tooltip"),
    TerminalCommand         UMETA(DisplayName = "Terminal Command"),
    InteractRegistered      UMETA(Hidden, DisplayName = "Interact Registered"),
    InteractUnregistered    UMETA(Hidden, DisplayName = "Interact Unregistered"),

    // ========================================================================
    // Команды жизненного цикла терминала
    // ========================================================================
    RegisterTerminalRequest   UMETA(DisplayName = "Register Terminal Request"),
    UnregisterTerminalRequest UMETA(DisplayName = "Unregister Terminal Request"),
    SetCapabilitiesRequest    UMETA(DisplayName = "Set Capabilities Request"),
    ApplyProfileRequest       UMETA(DisplayName = "Apply Profile Request"),
    ResetToDefaultRequest     UMETA(DisplayName = "Reset To Default Request"),

    // ========================================================================
    // Команды доступа / аутентификации
    // ========================================================================
    OpenTerminalRequest     UMETA(DisplayName = "Open Terminal Request"),
    CloseTerminalRequest    UMETA(DisplayName = "Close Terminal Request"),
    LoginPasswordRequest    UMETA(DisplayName = "Login Password Request"),
    LoginAccountRequest     UMETA(DisplayName = "Login Account Request"),
    ContextualAccessRequest UMETA(DisplayName = "Contextual Access Request"),

    // ========================================================================
    // Команды файловых операций
    // ========================================================================
    WriteFileRequest        UMETA(DisplayName = "Write File Request"),
    DeleteFileRequest       UMETA(DisplayName = "Delete File Request"),

    // ========================================================================
    // Команды глобальных сервисов
    // ========================================================================
    SetEmailContentRequest   UMETA(DisplayName = "Set Email Content Request"),
    SetWebsiteContentRequest UMETA(DisplayName = "Set Website Content Request"),
    SetGlobalDataRequest     UMETA(DisplayName = "Set Global Data Request"),
    AddLogRequest            UMETA(DisplayName = "Add Log Request"),

    // ========================================================================
    // Игры: ВХОДЯЩИЕ команды (внешний код → подсистема)
    //
    // Публикует терминал (или квест, диалог и т.п.).
    // Слушает UTerminalSubsystem.
    // ========================================================================
    GameStarted         UMETA(DisplayName = "Game Started"),
    GameStageFinished   UMETA(DisplayName = "Game Stage Finished"),
    GameCompleted       UMETA(DisplayName = "Game Completed"),
    GameStageRemoved    UMETA(DisplayName = "Game Stage Removed"),
    GameRecordRemoved   UMETA(DisplayName = "Game Record Removed"),

    // ========================================================================
    // Игры: ИСХОДЯЩИЕ отчёты (подсистема → наружу)
    //
    // Публикует только UTerminalSubsystem после обработки соответствующей
    // входящей команды. Слушают UI, Chore, Quest и прочие подписчики.
    // ========================================================================
    ReportGameStarted        UMETA(DisplayName = "Report Game Started"),
    ReportGameStageFinished  UMETA(DisplayName = "Report Game Stage Finished"),
    ReportGameStageRemoved   UMETA(DisplayName = "Report Game Stage Removed"),
    ReportGameCompleted      UMETA(DisplayName = "Report Game Completed"),
    ReportGameRecordRemoved  UMETA(DisplayName = "Report Game Record Removed")
};

UENUM(BlueprintType)
enum class EOutcomeInterior : uint8
{
    Default                     UMETA(DisplayName = "Default"),
    //InteriorEntered         UMETA(DisplayName = "Interior Entered"),
    //InteriorTransition      UMETA(DisplayName = "Interior Transition"),
    //InteriorLeaving         UMETA(DisplayName = "Interior Leaving"),
    //FloorEntered            UMETA(DisplayName = "Floor Entered"),
    FloorTransition             UMETA(DisplayName = "Floor Transition"),
    FloorLeaving                UMETA(DisplayName = "Floor Leaving"),
    //BuildingEntered         UMETA(DisplayName = "Location Entered"),
    //BuildingTransition      UMETA(DisplayName = "Location Transition"),
    //BuildingLeaving         UMETA(DisplayName = "Location Leaving"),
    InteractSetEnabled          UMETA(DisplayName = "Interact Set Enabled"),
    InteractSetRange            UMETA(DisplayName = "Interact Set Range"),
    InteractSetTooltip          UMETA(DisplayName = "Interact Set Tooltip"),
    InteractRegistered          UMETA(Hidden, DisplayName = "Interact Registered"),
    InteractUnregistered        UMETA(Hidden, DisplayName = "Interact Unregistered"),
    FloorPlacementRegistered    UMETA(Hidden, DisplayName = "Floor Placement Registered"),
    FloorPlacementUnregistered  UMETA(Hidden, DisplayName = "Floor Placement Unregistered"),
    FloorStateSave              UMETA(DisplayName = "Floor State Save"),
    FloorStateRestore           UMETA(DisplayName = "Floor State Restore"),
    LevelLoaded                 UMETA(DisplayName = "Level Loaded"),
    LocationVisitReset          UMETA(DisplayName = "Location Visit Reset"),
    CheckRequest                UMETA(DisplayName = "Check Request"),
    CheckResponse               UMETA(DisplayName = "Check Response"),
    StreetTransition            UMETA(DisplayName = "Street Transition"),
    ResetSpawnerSnapshot        UMETA(DisplayName = "Reset Spawner Snapshot"),
    SpawnerPreserveModeQuery    UMETA(DisplayName = "Spawner Preserve Mode Query")
    //CheckRequest            UMETA(DisplayName = "Check Request"),
    //CheckResponse           UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeSpawnGroup : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    SpawnGroupActivated     UMETA(DisplayName = "Spawn Group Activated"),
    SpawnGroupCleared       UMETA(DisplayName = "Spawn Group Cleared"),
    SpawnGroupDisabled      UMETA(DisplayName = "Spawn Group Disabled"),
    SpawnGroupReset         UMETA(DisplayName = "Spawn Group Reset"),
    SpawnGroupRegister      UMETA(DisplayName = "Spawn Group Register"),
    SpawnGroupUnregister    UMETA(DisplayName = "Spawn Group Unregister"),
    GhostCaptured           UMETA(DisplayName = "Ghost Captured"),

    SpawnGroupConfigure         UMETA(DisplayName = "Spawn Group Configure"),
    SpawnGroupDestroyRemaining  UMETA(DisplayName = "Spawn Group Destroy Remaining"),
    SpawnGroupSetManualActivation   UMETA(DisplayName = "Spawn Group Set Manual Activation")
};

UENUM(BlueprintType)
enum class EOutcomeWorldState : uint8
{
    Default                     UMETA(DisplayName = "Default"),
    WorldStateAddRecord         UMETA(DisplayName = "Adding State Record"),
    WorldStateRemoveRecord      UMETA(DisplayName = "Removing State Record"),
    WorldStateFactAdded         UMETA(DisplayName = "Fact Added"),
    WorldStateFactChanged       UMETA(DisplayName = "Fact Changed"),
    WorldStateFactRemoved       UMETA(DisplayName = "Fact Removed"),
	InteractionEnabledChanged   UMETA(DisplayName = "Interaction Enabled Changed"),
    //ChangingExteriorDoorAvailability UMETA(DisplayName = "Changing Exterior Door Availability"),
    //CheckRequest                UMETA(DisplayName = "Check Request"),
    //CheckResponse               UMETA(DisplayName = "Check Response")
};

UENUM(BlueprintType)
enum class EOutcomeChore : uint8
{
    Default                 UMETA(DisplayName = "Default"),
    ChoreOffered            UMETA(DisplayName = "Chore Offered"),
    ChoreAccepted           UMETA(DisplayName = "Chore Accepted"),
    ChoreStarted            UMETA(DisplayName = "Chore Started"),
    ChoreSucceeded          UMETA(DisplayName = "Chore Succeeded"),
    ChoreFailed             UMETA(DisplayName = "Chore Failed"),
    ChoreExpired            UMETA(DisplayName = "Chore Expired"),
    ChoreRetryAvailable     UMETA(DisplayName = "Chore Retry Available"),
    ChoreReactivated        UMETA(DisplayName = "Chore Reactivated"),
    ChoreRewardGranted      UMETA(DisplayName = "Chore Reward Granted"),
    ChoreAbandoned          UMETA(DisplayName = "Chore Abandoned"),
    ChoreMissionResult      UMETA(DisplayName = "Chore Mission Result"),
    AcceptRequest           UMETA(DisplayName = "Accept Request"),
    StartRequest            UMETA(DisplayName = "Start Request"),
    CompleteRequest         UMETA(DisplayName = "Complete Request"),
    FailRequest             UMETA(DisplayName = "Fail Request"),
    ExpireRequest           UMETA(DisplayName = "Expire Request"),
    AbandonRequest          UMETA(DisplayName = "Abandon Request"),
    RetryRequest            UMETA(DisplayName = "Retry Request"),
    UnlockRequest           UMETA(DisplayName = "Unlock Request"),
    RegisterChoreRequest    UMETA(DisplayName = "Register Chore Request"),
    UnregisterChoreRequest  UMETA(DisplayName = "Unregister Chore Request"),
    ReacceptRequest         UMETA(DisplayName = "Re-accept Request"),
    ChorePendingReaccept    UMETA(DisplayName = "Chore Pending Re-accept"),
    AdvanceStageRequest     UMETA(DisplayName = "Advance Stage Request"),
    PauseRequest            UMETA(DisplayName = "Pause Request"),
    ResumeRequest           UMETA(DisplayName = "Resume Request"),

    // ---- Нотификации от Chore Manager ----
    ChoreStageAdvanced      UMETA(DisplayName = "Chore Stage Advanced"),
    ChorePaused             UMETA(DisplayName = "Chore Paused"),
    ChoreResumed            UMETA(DisplayName = "Chore Resumed"),

    // ---- Deferred evaluation ----
    DeferredEvaluationRequest   UMETA(DisplayName = "Deferred Evaluation Request"),
    DeferredEvaluationQuery     UMETA(DisplayName = "Deferred Evaluation Query"),
    DeferredResultReport        UMETA(DisplayName = "Deferred Result Report"),
    ReplayDeferredResultRequest UMETA(DisplayName = "Replay Deferred Result Request"),
    DeferredResultReplay        UMETA(DisplayName = "Deferred Result Replay"),

    // Актор сообщает, что он появился на сцене
    RegisterChoreActor       UMETA(DisplayName = "Register Chore Actor"),
    // Актор сообщает, что он покидает сцену
    UnregisterChoreActor     UMETA(DisplayName = "Unregister Chore Actor")
};
