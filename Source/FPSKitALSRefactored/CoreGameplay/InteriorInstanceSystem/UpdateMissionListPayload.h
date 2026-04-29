#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionEnvelopeTypes.h"
#include "MissionSubsystem.h"
#include "UpdateMissionListPayload.generated.h"

/** Payload for UpdateMissionListPayload commands. */
// (Payload для команд UpdateMissionListPayload.)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UUpdateMissionListPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<FName, FActiveMissionEntry> ActiveMissions;
};