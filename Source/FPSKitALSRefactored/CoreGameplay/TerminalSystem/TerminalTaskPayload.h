#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "TerminalTaskPayload.generated.h"

// Payload for TerminalTask events
// (Payload для событий терминальных задач)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UTerminalTaskPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "TerminalTask")
	FGuid TerminalId;

	UPROPERTY(BlueprintReadWrite, Category = "TerminalTask")
	FString TaskId;

	UPROPERTY(BlueprintReadWrite, Category = "TerminalTask")
	bool bSuccess = false;

	UFUNCTION(BlueprintCallable, Category = "TerminalTask")
	UTerminalTaskPayload* Setup(
		const FGuid&   InTerminalId,
		const FString& InTaskId,
		bool           bInSuccess)
	{
		TerminalId = InTerminalId;
		TaskId     = InTaskId;
		bSuccess   = bInSuccess;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TerminalTask")
	FGuid GetTerminalId() const { return TerminalId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TerminalTask")
	FString GetTaskId() const { return TaskId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TerminalTask")
	bool IsSuccess() const { return bSuccess; }
};