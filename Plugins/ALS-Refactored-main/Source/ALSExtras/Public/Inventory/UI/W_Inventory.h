#pragma once

#include "Inventory/S_ItemData.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "W_Inventory.generated.h"

class UW_InventoryHUD;
class UAC_Container;
class UW_SlotContainer;
class UW_ItemSlot;
class UScrollBox;
class UTextBlock;

UCLASS()
class ALSEXTRAS_API UW_Inventory : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UW_InventoryHUD> InventoryHUDRef;

	UPROPERTY(BlueprintReadOnly, Category = "UI", meta = (ExposeOnSpawn = "true"))
	EnumInventoryType InventoryType;

	UPROPERTY(BlueprintReadWrite, Category = "UI", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UAC_Container> Container;

	UPROPERTY(BlueprintReadWrite, Category = "Items")
	TArray<TObjectPtr<UW_ItemSlot>> Slots;

	UPROPERTY(BlueprintReadWrite, Category = "ScrollBox field", meta = (BindWidget))
	UScrollBox* ScrollBox_Items;
	
	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_TotalArmour;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_TotalWeight;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_TotalMoney;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	EnumInventory CurrentTabType;

	UFUNCTION(BlueprintCallable, Category = "Filter")
	void SlotsFilter(EnumInventory SlotContainerType);

	UFUNCTION(BlueprintCallable, Category = "Params")
	void RefreshArmour(float Armour);

	UFUNCTION(BlueprintCallable, Category = "Params")
	void RefreshWeight(float Weight);

	UFUNCTION(BlueprintCallable, Category = "Params")
	void RefreshMoney(float Money);

	//sorting
private:
	uint8 bIsDecreased : 1{false};

	EnumSortType PrevSort;

protected:
	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Items_Sort(EnumSortType SortType, EnumInventory SlotContainerType);
};
