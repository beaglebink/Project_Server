#pragma once
#include "CoreMinimal.h"
#include "OutcomePayload.h"
#include "InteractItemStatePayload.generated.h"

// Payload sent by subsystem to update interaction availability and tooltip for a specific item
// Player receives this and does NOT need a direct reference to the component
// (Payload от подсистемы для обновления доступности интеракции и подсказки конкретного объекта)
// (Плейер получает это и НЕ нуждается в прямой ссылке на компонент)
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInteractItemStatePayload : public UOutcomePayload
{
	GENERATED_BODY()

public:
	// Which item this state update applies to
	// (К какому объекту применяется это обновление состояния)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItemState")
	FGuid ItemId;

	// Whether interaction is currently available
	// (Доступна ли интеракция в данный момент)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItemState")
	bool bInteractionEnabled = true;

	// Current tooltip text (may differ from default)
	// (Актуальная подсказка - может отличаться от дефолтной)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItemState")
	FText CurrentTooltip;

	// Current interaction range (subsystem may modify it at runtime)
	// (Актуальная дистанция интеракции - подсистема может изменить в рантайме)
	UPROPERTY(BlueprintReadWrite, Category = "InteractItemState")
	float InteractionRange = 200.f;

	UFUNCTION(BlueprintCallable, Category = "InteractItemState")
	UInteractItemStatePayload* Setup(
		const FGuid& InItemId,
		bool         bInEnabled,
		const FText& InTooltip,
		float        InRange)
	{
		ItemId              = InItemId;
		bInteractionEnabled = bInEnabled;
		CurrentTooltip      = InTooltip;
		InteractionRange    = InRange;
		return this;
	}

	// Getters
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItemState")
	FGuid GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItemState")
	bool IsInteractionEnabled() const { return bInteractionEnabled; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItemState")
	FText GetCurrentTooltip() const { return CurrentTooltip; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "InteractItemState")
	float GetInteractionRange() const { return InteractionRange; }
};