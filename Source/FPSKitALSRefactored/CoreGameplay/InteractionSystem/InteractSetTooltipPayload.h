#pragma once

#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteractSetTooltipPayload.generated.h"

/**
 * Payload для команды изменения текста подсказки (FText) через EventBus.
 */
UCLASS(BlueprintType)
class FPSKITALSREFACTORED_API UInteractSetTooltipPayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// ID интерактивного элемента — UInteractiveItemComponent::GetItemId()
	UPROPERTY(BlueprintReadWrite, Category = "InteractSetTooltip")
	FGuid ItemId;

	// Новый текст подсказки
	UPROPERTY(BlueprintReadWrite, Category = "InteractSetTooltip")
	FText NewTooltip;

	UFUNCTION(BlueprintCallable, Category = "InteractSetTooltip")
	UInteractSetTooltipPayload* Setup(const FGuid& InItemId, const FText& InTooltip)
	{
		ItemId = InItemId;
		NewTooltip = InTooltip;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetTooltip")
	FGuid GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractSetTooltip")
	FText GetNewTooltip() const { return NewTooltip; }
};