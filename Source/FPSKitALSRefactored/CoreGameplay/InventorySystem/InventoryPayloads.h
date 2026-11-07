#pragma once

#include "CoreMinimal.h"
#include "EventBusSystem/OutcomePayload.h"
#include "InventorySnapshotTypes.h"
#include "InventoryPayloads.generated.h"

// ═══════════════════════════════════════════════════════════════════════════
// UInventoryChangePayload
// Публикуется из UAC_Container::AddToContainer / RemoveFromContainer
// только для контейнера, принадлежащего AAlsCharacterExample.
// ═══════════════════════════════════════════════════════════════════════════
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UInventoryChangePayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") FName ItemName;
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") int32 Quantity = 0;
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") float TradeCoeff = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") bool  bShouldCount = false;
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") int32 NewQuantity = 0;
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") float CurrentWeight = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "InventoryChange") float CurrentMoney  = 0.0f;

    UFUNCTION(BlueprintCallable, Category = "InventoryChange")
    UInventoryChangePayload* Setup(
        FName InItemName,
        int32 InQuantity,
        float InTradeCoeff,
        bool  bInShouldCount,
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

// ═══════════════════════════════════════════════════════════════════════════
// UItemAcquiredPayload
// Публикуется для событий получения предметов.
// ActorId  — актор, инициировавший получение (обычно pawn игрока).
// ObjectId — идентификатор полученного объекта/предмета.
// ═══════════════════════════════════════════════════════════════════════════
UCLASS(BlueprintType, Blueprintable)
class FPSKITALSREFACTORED_API UItemAcquiredPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category = "ItemAcquired") FGuid ActorId;
    UPROPERTY(BlueprintReadWrite, Category = "ItemAcquired") FGuid ObjectId;

    UFUNCTION(BlueprintCallable, Category = "ItemAcquired")
    UItemAcquiredPayload* Setup(
        const FGuid& InActorId,
        const FGuid& InObjectId)
    {
        ActorId  = InActorId;
        ObjectId = InObjectId;
        return this;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ItemAcquired")
    FGuid GetActorId() const { return ActorId; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ItemAcquired")
    FGuid GetObjectId() const { return ObjectId; }
};

// ═══════════════════════════════════════════════════════════════════════════
// UInventoryContainerPayload
// Публикуется из UAC_Container::EndPlay для контейнеров в мире (не игрока).
// Используется в EOutcomeInventory::SaveContainerRequest.
// ═══════════════════════════════════════════════════════════════════════════
UCLASS()
class FPSKITALSREFACTORED_API UInventoryContainerPayload : public UOutcomePayload
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite) FGuid ItemId;
    UPROPERTY(BlueprintReadWrite) FInventorySnapshot Snapshot;
};