#pragma once

#include "Inventory/S_ItemData.h"
#include "Inventory/AC_Container.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_SlotContainer.generated.h"

class UW_ItemSlot;

UCLASS()
class ALSEXTRAS_API UW_SlotContainer : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UAC_Container> OtherContainer;

protected:
	UPROPERTY(BlueprintReadWrite, Category = "Items")
	TArray<TObjectPtr<UW_ItemSlot>> Slots;

private:
	uint8 bIs_A_Z_Sort : 1{false};

	uint8 bIs_Damage_Sort : 1{false};

	uint8 bIs_Armor_Sort : 1{false};

	uint8 bIs_Durability_Sort : 1{false};

	uint8 bIs_Weight_Sort : 1{false};

	uint8 bIs_Value_Sort : 1{false};


protected:
	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void A_Z_Sort();

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Damage_Sort();

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Armor_Sort();

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Durability_Sort();

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Weight_Sort();

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Value_Sort();
};
