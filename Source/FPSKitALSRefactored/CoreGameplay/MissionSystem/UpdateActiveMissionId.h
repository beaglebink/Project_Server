#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionEnvelopeTypes.h"
#include "MissionSubsystem.h"
#include "UpdateActiveMissionId.generated.h"

/** Payload for UpdateActiveMissionId commands. */
// (Payload для команд UpdateActiveMissionId.)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UUpdateActiveMissionId : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FName ActiveMissionId;
};