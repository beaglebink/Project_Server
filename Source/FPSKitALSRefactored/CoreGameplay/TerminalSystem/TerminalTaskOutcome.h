#pragma once
#include "CoreMinimal.h"
#include "OutcomeEventBase.h"
#include "TerminalTaskOutcome.generated.h"

USTRUCT(BlueprintType)
struct FTerminalTaskOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FGuid TerminalId;

	UPROPERTY(BlueprintReadWrite)
	FString TaskId;

	UPROPERTY(BlueprintReadWrite)
	bool bSuccess = false;
};
