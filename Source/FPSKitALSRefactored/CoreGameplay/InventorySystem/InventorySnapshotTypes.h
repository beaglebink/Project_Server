#pragma once

#include "CoreMinimal.h"
#include "Inventory/S_ItemData.h"
#include "InventorySnapshotTypes.generated.h"

/**
 * Снапшот состояния инвентаря / контейнера.
 * Используется и для игрока (SavedInventorySnapshot), и для контейнеров в мире (по FGuid).
 */
USTRUCT(BlueprintType)
struct FPSKITALSREFACTORED_API FInventorySnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TArray<FS_Item> Items;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    float TotalWeight = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    float TotalMoney = 0.f;

    void Reset()
    {
        Items.Empty();
        TotalWeight = 0.f;
        TotalMoney  = 0.f;
    }
};