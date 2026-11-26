#pragma once

#include "CoreMinimal.h"
#include "ChoreSystem/ChoreEnums.h"
#include "GameplayTagContainer.h"
#include "DA_OrderCatalog.h"
#include "O_OnlineOrderingStructures.generated.h"

USTRUCT(BlueprintType)
struct FOrderItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	FGameplayTag OrderItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering", meta = (ClampMin = 0))
	int32 OrderItemQuantity;
};

USTRUCT(BlueprintType)
struct FOnlineOrderingData
{
	GENERATED_BODY()

	UPROPERTY()
	EChoreStatus OrderStatus = EChoreStatus::Inactive;

	UPROPERTY()
	uint8 bWasStarted : 1{false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering", meta = (ClampMin = 0))
	int32 OrderMoneyLimit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	TArray<UDA_OrderCatalog*> OrderCatalogArray;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OnlineOrdering")
	TArray<FOrderItem> OrderArray;
};

USTRUCT(BlueprintType)
struct FOnlineOrderingSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TMap<FName, FOnlineOrderingData> OnlineOrderingDataMap;
};