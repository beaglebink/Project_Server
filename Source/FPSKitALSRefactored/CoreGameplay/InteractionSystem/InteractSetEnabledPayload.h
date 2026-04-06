#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteractSetEnabledPayload.generated.h"

/**
 * Payload для команды включения/выключения интерактивного компонента через EventBus.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteractSetEnabledPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// ID интерактивного элемента — UInteractiveItemComponent::GetItemId()
	UPROPERTY(BlueprintReadWrite, Category = "InteractSetEnabled")
	FGuid ItemId;

	// true = включить, false = выключить
	UPROPERTY(BlueprintReadWrite, Category = "InteractSetEnabled")
	bool bEnabled = true;

	UFUNCTION(BlueprintCallable, Category = "InteractSetEnabled")
	UInteractSetEnabledPayload* Setup(const FGuid& InItemId, bool bInEnabled)
	{
		ItemId   = InItemId;
		bEnabled = bInEnabled;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetEnabled")
	FGuid GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetEnabled")
	bool IsEnabled() const { return bEnabled; }
};