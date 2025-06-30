#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "S_ItemData.generated.h"

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

USTRUCT(BLueprintType)
struct ALSEXTRAS_API FS_Item
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FName Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 1;

	bool operator ==(const FS_Item& Item)
	{
		return Item.Name == Name;
	}
};

USTRUCT(BlueprintType, Blueprintable)
struct ALSEXTRAS_API FS_ItemData :public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	EnumInventory Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	UStaticMesh* StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (EditCondition = "Type == EnumInventory::Weapon", EditConditionHides))
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (EditCondition = "Type == EnumInventory::Clothes", EditConditionHides))
	float Armor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (EditCondition = "Type == EnumInventory::Weapon || Type == EnumInventory::Clothes", EditConditionHides))
	float Durability;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	float Weight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	float Value;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	uint8 bCanStack : 1 {false};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	FGameplayTag SpecialTag;
};
