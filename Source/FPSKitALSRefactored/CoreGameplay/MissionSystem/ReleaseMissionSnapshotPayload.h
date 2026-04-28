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

	// Причина завершения миссии (только при IsCompletion=true)
	UPROPERTY(BlueprintReadWrite, Category = "Mission")
	EMissionEndReason EndReason = EMissionEndReason::None;

	// true — вызван при завершении миссии (используем ExitPolicy.OnMissionCompleted)
	// false — вызван при выходе из здания во время активной миссии (используем JobSpacePolicy)
	UPROPERTY(BlueprintReadWrite, Category = "Mission")
	bool bIsCompletion = false;

	UFUNCTION(BlueprintCallable, Category = "Mission")
	UReleaseMissionSnapshotPayload* Setup(FName InMissionId, const FMissionEnvelope& InEnvelope, EJobSpacePolicy InPolicy)
	{
		MissionId = InMissionId;
		Envelope = InEnvelope;
		Policy = InPolicy;
		bIsCompletion = false;
		EndReason = EMissionEndReason::None;
		return this;
	}

	UFUNCTION(BlueprintCallable, Category = "Mission")
	UReleaseMissionSnapshotPayload* SetupCompletion(FName InMissionId, const FMissionEnvelope& InEnvelope, EJobSpacePolicy InPolicy, EMissionEndReason InEndReason)
	{
		MissionId = InMissionId;
		Envelope = InEnvelope;
		Policy = InPolicy;
		bIsCompletion = true;
		EndReason = InEndReason;
		return this;
	}
};