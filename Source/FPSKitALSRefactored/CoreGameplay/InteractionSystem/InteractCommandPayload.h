#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteractCommandPayload.generated.h"

class UInteractivePickerComponent;

/**
 * Payload for the "start interaction" command sent over EventBus.
 * (Payload для команды "начать интеракцию" через EventBus.)
 * Contains ItemId of the interactive item and a pointer to the Picker used for broadcast.
 * (Содержит ItemId интерактивного элемента и указатель на Picker для Broadcast.)
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteractCommandPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, Category = "InteractCommand")
	FGuid ItemId;

	// Picker, который инициировал интеракцию — передаётся в OnInteractionPressKeyEvent.Broadcast
	UPROPERTY(BlueprintReadWrite, Category = "InteractCommand")
	TObjectPtr<UInteractivePickerComponent> Picker = nullptr;
};