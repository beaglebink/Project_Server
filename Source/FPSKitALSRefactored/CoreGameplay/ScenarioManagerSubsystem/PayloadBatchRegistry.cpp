// PayloadBatchRegistry.cpp
#include "PayloadBatchRegistry.h"

// --- Пути подгони под свой проект. Если все заголовки лежат в разных
// --- папках модуля, указывай относительный путь от корня модуля.
#include "MissionSystem/MissionEnvelopePayload.h"
#include "MissionSystem/MissionProgressPayload.h"
#include "MissionSystem/ReleaseMissionSnapshotPayload.h"
#include "TerminalSystem/TerminalCommandPayload.h"
#include "TerminalSystem/TerminalTaskPayload.h"
#include "WorldStateSystem/WorldStateRecordPayload.h"
#include "WorldStateSystem/WorldStateRecordRemovePayload.h"
#include "WorldStateSystem/WorldStateFactChangedPayload.h"
#include "ChoreSystem/ChorePayloads.h"

// --- NEW: SpawnGroup payloads ---
#include "SpawnGroupSystem/SpawnGroupCommandPayload.h"
#include "SpawnGroupSystem/SpawnGroupConfigurePayload.h"

static FCommandSpec MakeMission(FName Name, TSubclassOf<UOutcomePayload> Class, EOutcomeMission Sub)
{
    FCommandSpec S;
    S.Subsystem = EBatchSubsystem::Mission;
    S.CommandName = Name;
    S.PayloadClass = Class;
    S.Route.OutcomeType = EOutcomeType::Mission;
    S.Route.OutcomeMission = Sub;
    return S;
}

static FCommandSpec MakeTerminal(FName Name, TSubclassOf<UOutcomePayload> Class, EOutcomeTerminal Sub)
{
    FCommandSpec S;
    S.Subsystem = EBatchSubsystem::Terminal;
    S.CommandName = Name;
    S.PayloadClass = Class;
    S.Route.OutcomeType = EOutcomeType::Terminal;
    S.Route.OutcomeTerminal = Sub;
    return S;
}

static FCommandSpec MakeWorldState(FName Name, TSubclassOf<UOutcomePayload> Class, EOutcomeWorldState Sub)
{
    FCommandSpec S;
    S.Subsystem = EBatchSubsystem::WorldState;
    S.CommandName = Name;
    S.PayloadClass = Class;
    S.Route.OutcomeType = EOutcomeType::WorldState;
    S.Route.OutcomeWorldState = Sub;
    return S;
}

static FCommandSpec MakeSpawnGroup(FName Name, TSubclassOf<UOutcomePayload> Class, EOutcomeSpawnGroup Sub)
{
    FCommandSpec S;
    S.Subsystem = EBatchSubsystem::SpawnGroup;
    S.CommandName = Name;
    S.PayloadClass = Class;
    S.Route.OutcomeType = EOutcomeType::SpawnGroup;
    S.Route.OutcomeSpawnGroup = Sub;
    return S;
}

static FCommandSpec MakeChore(FName Name, TSubclassOf<UOutcomePayload> Class, EOutcomeChore Sub)
{
    FCommandSpec S;
    S.Subsystem = EBatchSubsystem::Chore;
    S.CommandName = Name;
    S.PayloadClass = Class;
    S.Route.OutcomeType = EOutcomeType::Chore;
    S.Route.OutcomeChore = Sub;
    return S;
}

const TArray<FCommandSpec>& FPayloadBatchRegistry::GetSpecs()
{
    static const TArray<FCommandSpec> Specs =
    {
        // ========================= MISSION =========================
        MakeMission("Mission.Activate",        UMissionEnvelopePayload::StaticClass(),        EOutcomeMission::MissionActivated),
        //MakeMission("Mission.Update",          UMissionEnvelopePayload::StaticClass(),        EOutcomeMission::MissionUpdate),
        MakeMission("Mission.Progress",        UMissionProgressPayload::StaticClass(),        EOutcomeMission::MissionProgress),
        MakeMission("Mission.Complete",        UMissionEnvelopePayload::StaticClass(),        EOutcomeMission::MissionCompleted),
        //MakeMission("Mission.ReleaseSnapshot", UReleaseMissionSnapshotPayload::StaticClass(), EOutcomeMission::MissionCompleted),

        // ========================= TERMINAL ========================
        //MakeTerminal("Terminal.Register",         UTerminalRegisterCommandPayload::StaticClass(),        EOutcomeTerminal::RegisterTerminalRequest),
        //MakeTerminal("Terminal.Unregister",       UTerminalIdCommandPayload::StaticClass(),              EOutcomeTerminal::UnregisterTerminalRequest),
        //
        //MakeTerminal("Terminal.ApplyProfile",     UTerminalApplyProfileCommandPayload::StaticClass(),    EOutcomeTerminal::ApplyProfileRequest),
        //
        //MakeTerminal("Terminal.SetCapabilities",  UTerminalSetCapabilitiesCommandPayload::StaticClass(), EOutcomeTerminal::SetCapabilitiesRequest),
        //MakeTerminal("Terminal.LoginPassword",    UTerminalLoginPasswordCommandPayload::StaticClass(),   EOutcomeTerminal::LoginPasswordRequest),
        //MakeTerminal("Terminal.LoginAccount",     UTerminalLoginAccountCommandPayload::StaticClass(),    EOutcomeTerminal::LoginAccountRequest),

        //MakeTerminal("Terminal.ContextualAccess", UTerminalContextualAccessCommandPayload::StaticClass(),EOutcomeTerminal::ContextualAccessRequest),
        //MakeTerminal("Terminal.WriteFile",        UTerminalWriteFileCommandPayload::StaticClass(),       EOutcomeTerminal::WriteFileRequest),
        //MakeTerminal("Terminal.DeleteFile",       UTerminalDeleteFileCommandPayload::StaticClass(),      EOutcomeTerminal::DeleteFileRequest),
        //MakeTerminal("Terminal.SetEmail",         UTerminalSetEmailCommandPayload::StaticClass(),        EOutcomeTerminal::SetEmailContentRequest),
        //MakeTerminal("Terminal.SetWebsite",       UTerminalSetWebsiteCommandPayload::StaticClass(),      EOutcomeTerminal::SetWebsiteContentRequest),
        //MakeTerminal("Terminal.SetGlobalData",    UTerminalSetGlobalDataCommandPayload::StaticClass(),   EOutcomeTerminal::SetGlobalDataRequest),
        //MakeTerminal("Terminal.AddLog",           UTerminalAddLogCommandPayload::StaticClass(),          EOutcomeTerminal::AddLogRequest),

        // ======================= CHORE =======================
        MakeChore("Chore.Start",                    UChoreCommandPayload::StaticClass(),                    EOutcomeChore::StartRequest),
        MakeChore("Chore.Reaccept",                 UChoreCommandPayload::StaticClass(),                    EOutcomeChore::ReacceptRequest),
        MakeChore("Chore.Pause",                    UChoreCommandPayload::StaticClass(),                    EOutcomeChore::PauseRequest),
        MakeChore("Chore.Resume",                   UChoreCommandPayload::StaticClass(),                    EOutcomeChore::ResumeRequest),

        // ======================= SPAWN GROUP =======================
        // === NEW: команды от ScenarioManager ===
        MakeSpawnGroup("SpawnGroup.Configure",
            USpawnGroupConfigurePayload::StaticClass(),
            EOutcomeSpawnGroup::SpawnGroupConfigure),
        MakeSpawnGroup("SpawnGroup.Start",
            USpawnGroupCommandPayload::StaticClass(),
            EOutcomeSpawnGroup::SpawnGroupActivated),
        MakeSpawnGroup("SpawnGroup.DestroyRemaining",
            USpawnGroupCommandPayload::StaticClass(),
            EOutcomeSpawnGroup::SpawnGroupDestroyRemaining),
            // === /NEW ===

            // ======================= WORLD STATE =======================
            MakeWorldState("WorldState.InteractionEnable",      UInteractSetEnabledPayload::StaticClass(),    EOutcomeWorldState::InteractionEnabledChanged),
            MakeWorldState("WorldState.WorldStateAddRecord",    UWorldStateRecordPayload::StaticClass(),      EOutcomeWorldState::WorldStateAddRecord),
            MakeWorldState("WorldState.WorldStateRemoveRecord", UWorldStateRecordRemovePayload::StaticClass(),EOutcomeWorldState::WorldStateRemoveRecord)
            //MakeWorldState("WorldState.WorldStateFactAdded",   UWorldStateFactChangedPayload::StaticClass(), EOutcomeWorldState::WorldStateFactAdded),
            //MakeWorldState("WorldState.WorldStateFactChanged", UWorldStateFactChangedPayload::StaticClass(), EOutcomeWorldState::WorldStateFactChanged),
            //MakeWorldState("WorldState.WorldStateFactRemoved", UWorldStateFactChangedPayload::StaticClass(), EOutcomeWorldState::WorldStateFactRemoved)
    };
    return Specs;
}

TArray<FName> FPayloadBatchRegistry::GetCommandNames(EBatchSubsystem InSubsystem)
{
    TArray<FName> Out;
    for (const FCommandSpec& S : GetSpecs())
        if (S.Subsystem == InSubsystem)
            Out.Add(S.CommandName);
    return Out;
}

const FCommandSpec* FPayloadBatchRegistry::Find(EBatchSubsystem InSubsystem, FName CommandName)
{
    for (const FCommandSpec& S : GetSpecs())
        if (S.Subsystem == InSubsystem && S.CommandName == CommandName)
            return &S;
    return nullptr;
}

void FPayloadBatchRegistry::ApplyRouteToEvent(const FCommandRoute& Route, FOutcomeEventBase& Event)
{
    Event.OutcomeType = Route.OutcomeType;
    Event.OutcomeMission = Route.OutcomeMission;
    Event.OutcomeActor = Route.OutcomeActor;
    Event.OutcomeInventory = Route.OutcomeInventory;
    Event.OutcomeTerminal = Route.OutcomeTerminal;
    Event.OutcomeInterior = Route.OutcomeInterior;
    Event.OutcomeSpawnGroup = Route.OutcomeSpawnGroup;
    Event.OutcomeWorldState = Route.OutcomeWorldState;
    Event.OutcomeChore = Route.OutcomeChore;
    // Event.Payload заполняется снаружи (в ExecuteEntry).
    // Event.SourceStepId заполняется вызывающей стороной (UPayloadBatchAsset::ExecuteEntry).
}