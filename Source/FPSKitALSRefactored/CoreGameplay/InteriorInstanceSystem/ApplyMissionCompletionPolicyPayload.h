#pragma once
#include "OutcomePayload.h"
#include <MissionEnvelopeTypes.h>
#include "ApplyMissionCompletionPolicyPayload.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UApplyMissionCompletionPolicyPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	FName MissionId;
	FMissionEnvelope Envelope;
	EJobSpacePolicy Policy;
	EMissionEndReason EndReason;
};