#pragma once

#include "CoreMinimal.h"
#include "EventBusSystem/OutcomePayload.h"
#include "InventoryChangePayload.generated.h"

/**
 * Payload события изменения содержимого инвентаря персонажа.
 * Публикуется из UAC_Container::AddToContainer / UAC_Container::RemoveFromContainer
 * только для контейнера, принадлежащего AAlsCharacterExample.
 */
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInventoryChangePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    FName ItemName;

    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    float TradeCoeff = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    bool bShouldCount = false;

    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    int32 NewQuantity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    float CurrentWeight = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange")
    float CurrentMoney = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "InventoryChange")
    UInventoryChangePayload* Setup(
        FName InItemName,
        int32 InQuantity,
        float InTradeCoeff,
        bool bInShouldCount,
        int32 InNewQuantity,
        float InCurrentWeight,
        float InCurrentMoney)
    {
        ItemName      = InItemName;
        Quantity      = InQuantity;
        TradeCoeff    = InTradeCoeff;
        bShouldCount  = bInShouldCount;
        NewQuantity   = InNewQuantity;
        CurrentWeight = InCurrentWeight;
        CurrentMoney  = InCurrentMoney;
        return this;
    }
};