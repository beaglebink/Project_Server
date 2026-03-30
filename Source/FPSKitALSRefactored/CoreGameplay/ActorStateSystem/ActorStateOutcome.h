#pragma once
#include "CoreMinimal.h"
#include "OutcomeEventBase.h"
#include "ActorStateOutcome.generated.h"

USTRUCT(BlueprintType)
struct FActorStateOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGuid ActorId;

	UPROPERTY(BlueprintReadWrite)
	FString StateChangeType;

	UPROPERTY(BlueprintReadWrite)
	FString ContextId;

	UPROPERTY(BlueprintReadWrite)
	TMap<FString, FString> Payload;
};
