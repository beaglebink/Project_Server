#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "MissionEnvelopeTypes.h"
#include "FloorStatePayload.generated.h"

class UFloorAsset; // forward

/** Payload for FloorStateSave and FloorStateRestore commands. */
// (Payload для команд FloorStateSave и FloorStateRestore.)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UFloorStatePayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// runtime-friendly selector (preferred)
	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FString InteriorSetPath; // e.g. "/Game/Content/Location/InteriorSet_MyHouse" or just asset name "InteriorSet_MyHouse"

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	int32 FloorIndex = -1;

	// resolved GUIDs (filled by Setup)
	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FGuid FloorId;

	// Optional: mission identifier — if set, InteriorSubsystem will save/restore under mission snapshot
	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	FName MissionId;

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	EJobSpacePolicy Policy;

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	TArray<FEnvelopeChannelEntry> Channels;

	UPROPERTY(BlueprintReadWrite, Category = "FloorState")
	int32 CurrentMissionStep = -1;

    UFUNCTION(BlueprintCallable, Category = "FloorState")
    UFloorStatePayload* Setup(const FString& InInteriorSetPathOrName, int32 InFloorIndex);

    UFUNCTION(BlueprintCallable, Category = "FloorState")
    UFloorStatePayload* SetupFromFloorAsset(UFloorAsset* InFloorAsset);
};