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
class UW_FocusableButton;

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
	TArray<TObjectPtr<UW_ItemSlot>> ItemSlots;

	UPROPERTY(BlueprintReadWrite, Category = "ScrollBox field", meta = (BindWidget))
	UScrollBox* ScrollBox_Items;
	
	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_TotalArmour;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_TotalWeight;

	UPROPERTY(BlueprintReadWrite, Category = "Slot text field", meta = (BindWidget))
	UTextBlock* TextBlock_TotalMoney;

	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Tab_All;

	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Tab_Weapon;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Tab_Clothes;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Tab_Consumables;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Tab_Miscellaneous;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Tab_Others;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Sort_AZ;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Sort_Damage;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Sort_Armour;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Sort_Durability;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Sort_Weight;
	
	UPROPERTY(BlueprintReadWrite, Category = "Button", meta = (BindWidget))
	UW_FocusableButton* Button_Sort_Value;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Items")
	UDataTable* ItemDataTable;

public:
	UPROPERTY(BlueprintReadWrite, Category = "UI")
	EnumInventory CurrentTabType;

	UFUNCTION(BlueprintImplementableEvent)
	void RefreshAfterSort();

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
	UPROPERTY(BlueprintReadWrite, Category = "Sorting")
	EnumInventory ActiveTab;

	UFUNCTION(BlueprintCallable, Category = "Sorting")
	void Items_Sort(EnumSortType SortType, EnumInventory SlotContainerType);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Tab_All_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Tab_Weapon_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Tab_Clothes_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Tab_Consumables_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Tab_Miscellaneous_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Tab_Others_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Sort_AZ_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Sort_Damage_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Sort_Armour_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Sort_Durability_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Sort_Weight_Pressed();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void Handle_Button_Sort_Value_Pressed();
};
