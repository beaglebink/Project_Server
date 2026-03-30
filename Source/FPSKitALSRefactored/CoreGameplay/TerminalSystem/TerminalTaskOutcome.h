#pragma once
#include "OutcomeEventBase.h"
#include "TerminalTaskOutcome.generated.h"

// Outcome structure for terminal task completion (Структура результата для завершения терминального задания)
USTRUCT(BlueprintType)
struct FTerminalTaskOutcome : public FOutcomeEventBase
{
	GENERATED_BODY()

	// Terminal identifier (Идентификатор терминала)
	UPROPERTY(BlueprintReadWrite)
	FGuid TerminalId;

	// Task identifier string (Строка идентификатора задания)
	UPROPERTY(BlueprintReadWrite)
	FString TaskId;

	// Success flag for task completion (Флаг успеха для завершения задания)
	UPROPERTY(BlueprintReadWrite)
	bool bSuccess;
};
