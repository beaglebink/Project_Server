#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "S_ItemData.generated.h"

UENUM(BlueprintType)
enum class EnumInventory :uint8
{
	All 			UMETA(DisplayName = "All"),
	Weapon			UMETA(DisplayName = "Weapon"),
	Clothes			UMETA(DisplayName = "Clothes/Armour"),
	Consumables		UMETA(DisplayName = "Consumables"),
	Miscellaneous	UMETA(DisplayName = "Misc"),
	Others			UMETA(DisplayName = "Others")
};

UENUM(BlueprintType)
enum class EnumInventoryType :uint8
{
	Inventory	UMETA(DisplayName = "Inventory"),
	Chest		UMETA(DisplayName = "Chest"),
	Corpse		UMETA(DisplayName = "Corpse"),
	Vendor		UMETA(DisplayName = "Vendor")
};

USTRUCT(BlueprintType)
struct ALSEXTRAS_API FS_Item
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	FName Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 1;

	bool operator ==(const FS_Item& Item)const
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
