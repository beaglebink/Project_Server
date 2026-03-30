#pragma once
#include "CoreMinimal.h"
#include "OutcomeEventBase.h"
#include "GhostClearedOutcome.generated.h"

USTRUCT(BlueprintType)
struct FGhostClearedOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGuid MissionId;

	UPROPERTY(BlueprintReadWrite)
	FString GhostType;

	UPROPERTY(BlueprintReadWrite)
	FGuid InteriorSetId;

	UPROPERTY(BlueprintReadWrite)
	FGuid SpawnGroupId;
};
