#pragma once

#include "CoreMinimal.h"
#include "Outcome.h"
#include "OutcomePayload.h"
#include "OutcomeEventBase.h"
#include "PayloadBatchRegistry.generated.h"

UENUM(BlueprintType)
enum class EBatchSubsystem : uint8
{
    Mission     UMETA(DisplayName = "Mission"),
    Terminal    UMETA(DisplayName = "Terminal"),
    WorldState  UMETA(DisplayName = "World State"),
    Chore       UMETA(DisplayName = "Chore"),
    //Actor       UMETA(DisplayName = "Actor"),
    //Inventory   UMETA(DisplayName = "Inventory"),
    //Interior    UMETA(DisplayName = "Interior"),
    SpawnGroup  UMETA(DisplayName = "Spawn Group")
};

// Полный набор фильтров FOutcomeEventBase, который будет выставлен перед Publish.
struct FCommandRoute
{
    EOutcomeType        OutcomeType = EOutcomeType::Default;
    EOutcomeMission     OutcomeMission = EOutcomeMission::Default;
    EOutcomeActor       OutcomeActor = EOutcomeActor::Default;
    EOutcomeInventory   OutcomeInventory = EOutcomeInventory::Default;
    EOutcomeTerminal    OutcomeTerminal = EOutcomeTerminal::Default;
    EOutcomeInterior    OutcomeInterior = EOutcomeInterior::Default;
    EOutcomeSpawnGroup  OutcomeSpawnGroup = EOutcomeSpawnGroup::Default;
    EOutcomeWorldState  OutcomeWorldState = EOutcomeWorldState::Default;
    EOutcomeChore       OutcomeChore = EOutcomeChore::Default;
};

struct FCommandSpec
{
    EBatchSubsystem               Subsystem = EBatchSubsystem::Mission;
    FName                         CommandName;
    TSubclassOf<UOutcomePayload>  PayloadClass = nullptr;
    FCommandRoute                 Route;
};

class FPSKITALSREFACTORED_API FPayloadBatchRegistry
{
public:
    static const TArray<FCommandSpec>& GetSpecs();
    static TArray<FName>               GetCommandNames(EBatchSubsystem InSubsystem);
    static const FCommandSpec* Find(EBatchSubsystem InSubsystem, FName CommandName);

    // Раскладывает route в поля FOutcomeEventBase (Payload НЕ трогает).
    static void ApplyRouteToEvent(const FCommandRoute& Route, FOutcomeEventBase& Event);
};