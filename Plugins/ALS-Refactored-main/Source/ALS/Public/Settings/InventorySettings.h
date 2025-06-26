// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Engine/DataAsset.h"
#include "CoreMinimal.h"
#include "InventorySettings.generated.h"


UENUM(BlueprintType)
enum class EnumInventory :uint8
{
	None			UMETA(DisplayName = "None"),
	Weapon			UMETA(DisplayName = "Weapon"),
	Ammo			UMETA(DisplayName = "Ammo"),
	Clothes			UMETA(DisplayName = "Clothes"),
	Consumables		UMETA(DisplayName = "Consumables"),
	Misc			UMETA(DisplayName = "Misc")
};

USTRUCT(BlueprintType)
struct FInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventorySettings")
	EnumInventory InventoryType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventorySettings")
	float Quantity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventorySettings")
	float Weight;

};

UCLASS(BlueprintType, Blueprintable)
class ALS_API UInventorySettings :public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InventorySettings")
	FInventoryItem InventoryItem;

};
