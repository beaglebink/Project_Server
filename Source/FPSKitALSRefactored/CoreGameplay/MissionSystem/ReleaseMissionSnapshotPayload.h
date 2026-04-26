#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionEnvelopeTypes.h"
#include "MissionSystem/../MissionSystem/MissionEnvelopeTypes.h"
#include "ReleaseMissionSnapshotPayload.generated.h"

UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UReleaseMissionSnapshotPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "Mission")
	FName MissionId;

	UPROPERTY(BlueprintReadWrite, Category = "Mission")
	FMissionEnvelope Envelope;

	UPROPERTY(BlueprintReadWrite, Category = "Mission")
	EJobSpacePolicy Policy;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	UReleaseMissionSnapshotPayload* Setup(FName InMissionId, const FMissionEnvelope& InEnvelope, EJobSpacePolicy InPolicy)
	{
		MissionId = InMissionId;
		Envelope = InEnvelope;
		Policy = InPolicy;
		return this;
	}
};